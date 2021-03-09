#include "tracer.h"
#define TRACER_MAJOR	42
#define QUEUE_LIMIT	0x200000
#define DRBD_IS_VMALLOC_ADDR(a)	(((unsigned long) (a) >= VMALLOC_START) && ((unsigned long) (a) < VMALLOC_END)) 

int trace_count = 0;

struct cdev cdev;
struct list_head queue_head;
struct drbd_tracer_spin	tracer_drbd_spinlock;

static int tracer_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int tracer_release(struct inode *inode, struct file *file) {
	return 0;
}

static long tracer_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned long size = 0, i = 0, ret = 0;
	long bytes = 0 ;
	struct user_data *user_data;
	struct drbd_tracer *node = NULL;

	switch(cmd) {
		case TRACE_DRBD_DATA:
			user_data = (struct user_data *) arg;
			size = user_data->u_size;
			for (i = 0; i < size; i++) {
				node = NULL;
				node = trace_dequeue_data();
				if (node) {
					user_data->u_data[i].jiffies = node->k_data.jiffies;
					user_data->u_data[i].msg_type = node->k_data.msg_type;
					user_data->u_data[i].cmd = node->k_data.cmd;
					user_data->u_data[i].time_insec = node->k_data.time_insec;
					user_data->u_data[i].bi_size = node->k_data.bi_size;
					user_data->u_data[i].buf_ptr = node->k_data.buf_ptr;
					
					ret = copy_to_user(user_data->p_data_tbl[i], node->k_data.p_data, 
								sizeof(struct p_data));

					if (DRBD_IS_VMALLOC_ADDR(node->k_data.p_data)) {
						vfree(node->k_data.p_data);
					} else {
						kfree(node->k_data.p_data);
					}

					if(DRBD_IS_VMALLOC_ADDR(node)) {
						vfree(node);
					} else {
						kfree(node);
					}
					bytes++;
				}
			}
			if (bytes) {
				user_data->ret_val = bytes;
			}
			break;
		defualt:
			return -1;
	}
	return 0;
}

const struct file_operations tracer_fops = {
	.owner = THIS_MODULE,
	.open = tracer_open,
	.release = tracer_release,
	.unlocked_ioctl = tracer_ioctl
};

int __init tracer_init(void)
{
	int err;

	err = register_chrdev_region(MKDEV(TRACER_MAJOR, 0), 1, "tracer_driver");
	if (err != 0) {
		printk("load fail tracer_device_driver\n");
		return err;
	}

	cdev_init(&cdev, &tracer_fops);
	cdev_add(&cdev, MKDEV(TRACER_MAJOR, 0), 1);
	INIT_LIST_HEAD(&queue_head);
	spin_lock_init(&(tracer_drbd_spinlock.drbd_sp_spin));
	return 0;
}

void __exit tracer_cleanup(void)
{
	struct drbd_tracer *node = NULL;
	while(1) {
		drbdtracer_spinlock(tracer_drbd_spinlock);
		if (list_empty(&queue_head)) {
			drbdtracer_unspinlock(tracer_drbd_spinlock);
			break;
		}
		node = list_first_entry(&queue_head, struct drbd_tracer, queue_list);
		if (node) {
			list_del(&(node->queue_list));
		}
		trace_count--;
		drbdtracer_unspinlock(tracer_drbd_spinlock);
	}
	cdev_del(&cdev);
	unregister_chrdev_region(MKDEV(TRACER_MAJOR, 0), 1);
}

int trace_enqueue_data(struct trace_data *trace_data) {
	struct drbd_tracer *data = NULL;
 	data = kmalloc(sizeof(struct drbd_tracer), GFP_KERNEL);
	if (data == NULL) {
		data = vmalloc(sizeof(struct drbd_tracer));
		if (data == NULL) {
			return -ENOMEM;
		} else {
			memset(data, 0, sizeof(struct drbd_tracer));
		}
	} else {
		memset(data, 0, sizeof(struct drbd_tracer));
	}

	data->k_data.p_data = kmalloc(sizeof(struct p_data), GFP_KERNEL);
	if (data->k_data.p_data == NULL) {
		data->k_data.p_data = vmalloc(sizeof(struct p_data));
		if (data->k_data.p_data == NULL) {
			return -ENOMEM;
		} else {
			memset(data->k_data.p_data, 0, sizeof(struct p_data));
		}
	} else {
		memset(data->k_data.p_data, 0, sizeof(struct p_data));
	}

	data->k_data.jiffies = trace_data->jiffies;
	data->k_data.msg_type = trace_data->msg_type;
	data->k_data.cmd = trace_data->cmd;
	data->k_data.time_insec = trace_data->time_insec;
	data->k_data.bi_size = trace_data->bi_size;
	memcpy(data->k_data.p_data, trace_data->p_data, sizeof(struct p_data));
	data->k_data.buf_ptr = trace_data->buf_ptr;

	drbdtracer_spinlock(tracer_drbd_spinlock);
	if (trace_count < QUEUE_LIMIT) {
		trace_count++;
	} else {
		drbdtracer_unspinlock(tracer_drbd_spinlock);
		printk("drbd_tracer: QUEUE IS FULL\n");
		return -1;
	}
	list_add_tail(&data->queue_list, &queue_head);
	drbdtracer_unspinlock(tracer_drbd_spinlock);
	return 0;
}

struct drbd_tracer *trace_dequeue_data(void) {

	struct drbd_tracer *node = NULL;

	drbdtracer_spinlock(tracer_drbd_spinlock);
	if (list_empty(&queue_head)) {
		drbdtracer_unspinlock(tracer_drbd_spinlock);
		return NULL;
	}

	node = list_first_entry(&queue_head, struct drbd_tracer, queue_list);
	if (node) {
		list_del(&(node->queue_list));
	}
	trace_count--;
	drbdtracer_unspinlock(tracer_drbd_spinlock);
	return node;
}
EXPORT_SYMBOL(trace_enqueue_data);

MODULE_LICENSE("GPL");
module_init(tracer_init);
module_exit(tracer_cleanup);
