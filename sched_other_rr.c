/*
 * SCHED_OTHER_RR scheduling class. Implements a round robin scheduler with no
 * priority mechanism.
 */

/*
 * Update the current task's runtime statistics. Skip current tasks that
 * are not in our scheduling class.
 */

#include <linux/list.h>

static void update_curr_other_rr(struct rq *rq)
{
	struct task_struct *curr = rq->curr;
	u64 delta_exec;

	if (!task_has_other_rr_policy(curr))
		return;

	delta_exec = rq->clock - curr->se.exec_start;
	if (unlikely((s64)delta_exec < 0))
		delta_exec = 0;

	schedstat_set(curr->se.exec_max, max(curr->se.exec_max, delta_exec));

	curr->se.sum_exec_runtime += delta_exec;
	curr->se.exec_start = rq->clock;
	cpuacct_charge(curr, delta_exec);
}

/*
 * Adding/removing a task to/from a priority array:
 */
static void enqueue_task_other_rr(struct rq *rq, struct task_struct *p, int wakeup, bool b)
{
	update_curr_other_rr(rq);

	list_add_tail(&p->other_rr_run_list, &rq->other_rr.queue);
	rq->other_rr.nr_running++;
}

static void dequeue_task_other_rr(struct rq *rq, struct task_struct *p, int sleep)
{
	// first update the task's runtime statistics
	update_curr_other_rr(rq);

	list_del(&p->other_rr_run_list);
	rq->other_rr.nr_running--;
}

/*
 * Put task to the end of the run list without the overhead of dequeue
 * followed by enqueue.
 */
static void requeue_task_other_rr(struct rq *rq, struct task_struct *p)
{
	list_move_tail(&p->other_rr_run_list, &rq->other_rr.queue);
}

/*
 * current process is relinquishing control of the CPU
 */
static void
yield_task_other_rr(struct rq *rq)
{
	requeue_task_other_rr(rq, &rq->other_rr.queue);
}

/*
 * Preempt the current task with a newly woken task if needed:
 * int wakeflags added to match function signature of other schedulers
 */
static void check_preempt_curr_other_rr(struct rq *rq, struct task_struct *p, int wakeflags)
{
}

/*
 * select the next task to run
 */
static struct task_struct *pick_next_task_other_rr(struct rq *rq)
{
	struct task_struct *next;
	struct list_head *queue;
	struct other_rr_rq *other_rr_rq;

	queue = rq->other_rr.queue;
	next = queue->next;

	/* after selecting a task, we need to set a timer to maintain correct
	 * runtime statistics. You can uncomment this line after you have
	 * written the code to select the appropriate task.
	 */
	next->se.exec_start = rq->clock;
	
	/* you need to return the selected task here */
	return next;
}

static void put_prev_task_other_rr(struct rq *rq, struct task_struct *p)
{
	update_curr_other_rr(rq);
	p->se.exec_start = 0;
}

#ifdef CONFIG_SMP
/*
 * Load-balancing iterator. Note: while the runqueue stays locked
 * during the whole iteration, the current task might be
 * dequeued so the iterator has to be dequeue-safe. Here we
 * achieve that by always pre-iterating before returning
 * the current task:
 */
static struct task_struct *load_balance_start_other_rr(void *arg)
{
	struct rq *rq = arg;
	struct list_head *head, *curr;
	struct task_struct *p;

	head = &rq->other_rr.queue;
	curr = head->prev;

	p = list_entry(curr, struct task_struct, other_rr_run_list);

	curr = curr->prev;

	rq->other_rr.other_rr_load_balance_head = head;
	rq->other_rr.other_rr_load_balance_curr = curr;

	return p;
}

static struct task_struct *load_balance_next_other_rr(void *arg)
{
	struct rq *rq = arg;
	struct list_head *curr;
	struct task_struct *p;

	curr = rq->other_rr.other_rr_load_balance_curr;

	p = list_entry(curr, struct task_struct, other_rr_run_list);
	curr = curr->prev;
	rq->other_rr.other_rr_load_balance_curr = curr;

	return p;
}

static unsigned long
load_balance_other_rr(struct rq *this_rq, int this_cpu, struct rq *busiest,
		unsigned long max_load_move,
		struct sched_domain *sd, enum cpu_idle_type idle,
		int *all_pinned, int *this_best_prio)
{
	struct rq_iterator other_rr_rq_iterator;

	other_rr_rq_iterator.start = load_balance_start_other_rr;
	other_rr_rq_iterator.next = load_balance_next_other_rr;
	/* pass 'busiest' rq argument into
	 * load_balance_[start|next]_other_rr iterators
	 */
	other_rr_rq_iterator.arg = busiest;

	return balance_tasks(this_rq, this_cpu, busiest, max_load_move, sd,
			     idle, all_pinned, this_best_prio, &other_rr_rq_iterator);
}

static int
move_one_task_other_rr(struct rq *this_rq, int this_cpu, struct rq *busiest,
		 struct sched_domain *sd, enum cpu_idle_type idle)
{
	struct rq_iterator other_rr_rq_iterator;

	other_rr_rq_iterator.start = load_balance_start_other_rr;
	other_rr_rq_iterator.next = load_balance_next_other_rr;
	other_rr_rq_iterator.arg = busiest;

	return iter_move_one_task(this_rq, this_cpu, busiest, sd, idle,
				  &other_rr_rq_iterator);
}
#endif

/*
 * task_tick_other_rr is invoked on each scheduler timer tick.
 */
static void task_tick_other_rr(struct rq *rq, struct task_struct *p,int queued)
{
	// first update the task's runtime statistics
	update_curr_other_rr(rq);

	int quantum = sched_other_rr_getquantum;

	if(other_rr_time_slice != 0)
	{
		if(p->tast_time_slice != 0)
		{
			p->task_time_slice--;
		}
		else
		{
			p->task_time_slice = other_rr_time_slice;
			set_tsk_need_resched(p);
			requeue_task_other_rr(rq, p);
		}
	}
}

/*
 * scheduling policy has changed -- update the current task's scheduling
 * statistics
 */
static void set_curr_task_other_rr(struct rq *rq)
{
	struct task_struct *p = rq->curr;
	p->se.exec_start = rq->clock;
}

/*
 * We switched to the sched_other_rr class.
 */
static void switched_to_other_rr(struct rq *rq, struct task_struct *p,
			     int running)
{
	/*
	 * Kick off the schedule if running, otherwise just see
	 * if we can still preempt the current task.
	 */
	if (running)
		resched_task(rq->curr);
	else
		check_preempt_curr(rq, p, 0);
}

static int
select_task_rq_other_rr(struct rq *rq, struct task_struct *p, int sd_flag, int flags)
{
	if (sd_flag != SD_BALANCE_WAKE)
		return smp_processor_id();

	return task_cpu(p);
}

const struct sched_class other_rr_sched_class = {
	.next			= &idle_sched_class,
	.enqueue_task		= enqueue_task_other_rr,
	.dequeue_task		= dequeue_task_other_rr,
	.yield_task		= yield_task_other_rr,

	.check_preempt_curr	= check_preempt_curr_other_rr,

	.pick_next_task		= pick_next_task_other_rr,
	.put_prev_task		= put_prev_task_other_rr,

#ifdef CONFIG_SMP
	.load_balance		= load_balance_other_rr,
	.move_one_task		= move_one_task_other_rr,
#endif

	.switched_to  = switched_to_other_rr,
	.select_task_rq = select_task_rq_other_rr,

	.set_curr_task          = set_curr_task_other_rr,
	.task_tick		= (void *)task_tick_other_rr,
};
