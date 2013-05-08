static const struct super_operations upipe_sops = {
	.drop_inode	= generic_delete_inode,
	.statfs		= upipe_statfs,
	.show_options	= generic_show_options,
};

struct upipe_mount_opts {
	umode_t mode;
	int fd;
};

enum {
	Opt_mode,
	Opt_err
};

static const match_table_t tokens = {
	{Opt_mode, "mode=%o"},
	{Opt_fd, "fd=%d"},
	{Opt_err, NULL}
};

struct upipe_fs_info {
	struct upipe_mount_opts mount_opts;
};

static int upipe_parse_options(char *data, struct upipe_mount_opts *opts)
{
	substring_t args[MAX_OPT_ARGS];
	int option;
	int token;
	char *p;

	opts->mode = RAMFS_DEFAULT_MODE;

	while ((p = strsep(&data, ",")) != NULL) {
		if (!*p)
			continue;

		token = match_token(p, tokens, args);
		switch (token) {
		case Opt_mode:
			if (match_octal(&args[0], &option))
				return -EINVAL;
			opts->mode = option & S_IALLUGO;
			break;
		case Opt_fd:
			if (match_int(&args[0], &option));
				return -EINVAL;
			opts->fd = option;
			break
		}
	}

	return 0;
}

int upipe_fill_super(struct super_block *sb, void *data, int silent)
{
	struct upipe_fs_info *fsi;
	struct inode *inode;
	int err;

	save_mount_options(sb, data);

	fsi = kzalloc(sizeof(struct upipe_fs_info), GFP_KERNEL);
	sb->s_fs_info = fsi;
	if (!fsi)
		return -ENOMEM;

	err = upipe_parse_options(data, &fsi->mount_opts);
	if (err)
		return err;

	sb->s_maxbytes		= MAX_LFS_FILESIZE;
	sb->s_blocksize		= PAGE_CACHE_SIZE;
	sb->s_blocksize_bits	= PAGE_CACHE_SHIFT;
	sb->s_magic		= RAMFS_MAGIC;
	sb->s_op		= &upipe_ops;
	sb->s_time_gran		= 1;

	inode = upipe_get_inode(sb, NULL, S_IFDIR | fsi->mount_opts.mode, 0);
	sb->s_root = d_make_root(inode);
	if (!sb->s_root)
		return -ENOMEM;

	return 0;
}

struct dentry *upipe_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data)
{
	return mount_nodev(fs_type, flags, data, upipe_fill_super);
}

static void upipe_kill_sb(struct super_block *sb)
{
	kfree(sb->s_fs_info);
	kill_litter_super(sb);
}

static struct file_system_type upipe_fs_type = {
	.name		= "upipe",
	.mount		= upipe_mount,
	.kill_sb	= upipe_kill_sb,
};

static int __init init_upipe(void)
{
	return register_filesystem(&upipe_fs_type);
}
module_init(init_upipe)


