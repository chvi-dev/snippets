
#ifndef VssX_H_
#define VssX_H_

#ifndef __KERNEL__
#include <stdint.h>
#endif

#include <linux/ioctl.h>
#include <linux/limits.h>

#define VssX_VERSION "1.0.0.2"
#define VSS_IOCTL_X 0x1210

struct setup_params{
	char *bdev; //name of block device to snapshot
	char *cow; //name of cow file for snapshot
	unsigned long fallocated_space; //space allocated to the cow file (in megabytes)
	unsigned long cache_size; //maximum cache size (in bytes)
	unsigned int minor; //requested minor number of the device
};

struct reload_params{
	char *bdev; //name of block device to snapshot
	char *cow; //name of cow file for snapshot
	unsigned long cache_size; //maximum cache size (in bytes)
	unsigned int minor; //requested minor number of the device
};

struct transition_snap_params{
	char *cow; //name of cow file for snapshot
	unsigned long fallocated_space; //space allocated to the cow file (in bytes)
	unsigned int minor; //requested minor
};

struct reconfigure_params{
	unsigned long cache_size; //maximum cache size (in bytes)
	unsigned int minor; //requested minor number of the device
};

#define COW_UUID_SIZE 16
#define COW_BLOCK_LOG_SIZE 12
#define COW_BLOCK_SIZE (1 << COW_BLOCK_LOG_SIZE)
#define COW_HEADER_SIZE 4096
#define COW_X ((uint32_t)4776)
#define COW_CLEAN 0
#define COW_INDEX_ONLY 1
#define COW_VMALLOC_UPPER 2

#define COW_VERSION_0 0
#define COW_VERSION_CHANGED_BLOCKS 1

struct cow_header{
	uint32_t magic; //COW header magic
	uint32_t flags; //COW file flags
	uint64_t fpos; //current file offset
	uint64_t fsize; //file size
	uint64_t seqid; //seqential id of snapshot (starts at 1)
	uint8_t uuid[COW_UUID_SIZE]; //uuid for this series of snapshots
	uint64_t version; //version of cow file format
	uint64_t nr_changed_blocks; //number of changed blocks since last snapshot
};

struct vss_x_info{
	unsigned int minor;
	unsigned long state;
	int error;
	unsigned long cache_size;
	unsigned long long falloc_size;
	unsigned long long seqid;
	char uuid[COW_UUID_SIZE];
	char cow[PATH_MAX];
	char bdev[PATH_MAX];
	unsigned long long version;
	unsigned long long nr_changed_blocks;
};

#define IOCTL_SETUP_SNAP _IOW(VSS_IOCTL_X, 1, struct setup_params) //in: see above
#define IOCTL_RELOAD_SNAP _IOW(VSS_IOCTL_X, 2, struct reload_params) //in: see above
#define IOCTL_RELOAD_INC _IOW(VSS_IOCTL_X, 3, struct reload_params) //in: see above
#define IOCTL_DESTROY _IOW(VSS_IOCTL_X, 4, unsigned int) //in: minor
#define IOCTL_TRANSITION_INC _IOW(VSS_IOCTL_X, 5, unsigned int) //in: minor
#define IOCTL_TRANSITION_SNAP _IOW(VSS_IOCTL_X, 6, struct transition_snap_params) //in: see above
#define IOCTL_RECONFIGURE _IOW(VSS_IOCTL_X, 7, struct reconfigure_params) //in: see above
#define IOCTL_VSS_INFO _IOR(VSS_IOCTL_X, 8, struct vss_x_info) //in: see above
#define IOCTL_GET_FREE _IOR(VSS_IOCTL_X, 9, int)

#endif /* VssX_H_ */
