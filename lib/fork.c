// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;addr=addr;
	uint32_t err = utf->utf_err;err=err;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 9: Your code here.
//	panic("pgfault not implemented");

	pte_t pte = uvpt[PGNUM(addr)];
	if (!(err & FEC_WR)) {
        panic("ERROR pgfault");
	}
	if (!(pte & PTE_COW)) {
        panic("ERROR pgfault");
    }

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 9: Your code here.
    if (sys_page_alloc(0, PFTEMP, PTE_P | PTE_U | PTE_W) < 0) {
        panic("ERROR sys_page_alloc");
    }

    void *addr_rounded = ROUNDDOWN(addr, PGSIZE);
    memmove(PFTEMP, addr_rounded, PGSIZE);

    if (sys_page_map(0, PFTEMP, 0, addr_rounded, PTE_P | PTE_U | PTE_W) < 0) {
        panic("ERRROR sys_page_map");
    }

}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	// LAB 9: Your code here.
//	panic("duppage not implemented");
	pte_t pte = uvpt[pn];
	void *va = (void *)(pn * PGSIZE);
	if (pte & PTE_SHARE) {
		if (sys_page_map(0, va, envid, va, PTE_SYSCALL) < 0) {
			panic("duppage, sys_page_map shared");
		}
	} else if ((pte & PTE_W) || (pte & PTE_COW)) {
		if (sys_page_map(0, va, envid, va, PTE_P | PTE_U | PTE_COW) < 0) {
            panic("ERROR sys_page_map");
        }
		if (sys_page_map(0, va, 0, va, PTE_P | PTE_U | PTE_COW) < 0) {
            panic("ERROR sys_page_map");
        }
	} else {
		if (sys_page_map(0, va, envid, va, PTE_P | PTE_U) < 0) {
            panic("ERROR duppage, sys_page_map read only");
        }
	}

	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 9: Your code here.
	//panic("fork not implemented");
	set_pgfault_handler(pgfault);

	envid_t envid = sys_exofork();
	if (envid < 0) {
		panic("ERROR sys_exofork: %e", (double) envid);
	} else if (envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	for (int pde = 0; pde < PDX(UTOP); pde++) {
		if (!(uvpd[pde] & PTE_P))
			continue;

		for (int pte = 0; pte < NPTENTRIES; pte++) {
            uint32_t pgnum = (pde << 10) | pte;
			if (pgnum == PGNUM(UXSTACKTOP - PGSIZE)) {
				if (sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_U | PTE_P | PTE_W) < 0) {
                    panic("ERROR sys_page_alloc");
                }
				continue;
			}

			if (uvpt[pgnum] & PTE_P) {
				if (duppage(envid, pgnum) < 0) {
                    panic("ERROR duppage");
                }
			}
		}
	}

	if (sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall) < 0)
		panic("ERROR sys_env_set_pgfault_upcall");

	if (sys_env_set_status(envid, ENV_RUNNABLE) < 0)
		panic("ERROR sys_env_set_status");

	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
