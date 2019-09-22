#ifndef _LINUX_PID_NS_H
#define _LINUX_PID_NS_H

#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/threads.h>
#include <linux/nsproxy.h>
#include <linux/kref.h>


//分配pid的位图
struct pidmap {
       atomic_t nr_free;   //还能分配pid的数量
       void *page;         //指向存放pid的物理页
};

#define PIDMAP_ENTRIES         ((PID_MAX_LIMIT + 8*PAGE_SIZE - 1)/PAGE_SIZE/8)

struct pid_namespace {
	struct kref kref;
	struct pidmap pidmap[PIDMAP_ENTRIES];
	int last_pid;
	struct task_struct *child_reaper;     //每个PID命名空间都具有一个进程，相当于全局init进程。init的一个目的就是对孤儿进程调用wait4，命名空间的局部init变体也必须完成该工作，child_reaper保存了指向该进程的指针。
	struct kmem_cache *pid_cachep;
	int level;                      //当前命名空间在命名空间层次结构中的深度，初始level=0
	struct pid_namespace *parent;   //指向父命名空间的指针
#ifdef CONFIG_PROC_FS
	struct vfsmount *proc_mnt;
#endif
};

extern struct pid_namespace init_pid_ns;

#ifdef CONFIG_PID_NS
static inline struct pid_namespace *get_pid_ns(struct pid_namespace *ns)
{
	if (ns != &init_pid_ns)
		kref_get(&ns->kref);
	return ns;
}

extern struct pid_namespace *copy_pid_ns(unsigned long flags, struct pid_namespace *ns);
extern void free_pid_ns(struct kref *kref);

static inline void put_pid_ns(struct pid_namespace *ns)
{
	if (ns != &init_pid_ns)
		kref_put(&ns->kref, free_pid_ns);
}

#else /* !CONFIG_PID_NS */
#include <linux/err.h>

static inline struct pid_namespace *get_pid_ns(struct pid_namespace *ns)
{
	return ns;
}

static inline struct pid_namespace *
copy_pid_ns(unsigned long flags, struct pid_namespace *ns)
{
	if (flags & CLONE_NEWPID)
		ns = ERR_PTR(-EINVAL);
	return ns;
}

static inline void put_pid_ns(struct pid_namespace *ns)
{
}

#endif /* CONFIG_PID_NS */

static inline struct pid_namespace *task_active_pid_ns(struct task_struct *tsk)
{
	return tsk->nsproxy->pid_ns;
}

static inline struct task_struct *task_child_reaper(struct task_struct *tsk)
{
	BUG_ON(tsk != current);
	return tsk->nsproxy->pid_ns->child_reaper;
}

#endif /* _LINUX_PID_NS_H */
