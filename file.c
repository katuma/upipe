static int upipe_fsync(struct file *file, loff_t start, loff_t end, int datasync)
{
	struct inode *inode = file->f_mapping->host;
	ret = filemap_write_and_wait_range(inode->i_mapping, start, end);
	if (ret)
		return ret;

	PIPE_REQ;

	OP(FSYNC, inode);

	SUBMIT_REQ;
}

int upipe_readdir(struct file *file, void *ent, filldir_t filldir)
{
	struct inode *inode = file->f_path.dentry->d_inode;

	OP(READDIR, inode);
	PUT32(file->f_pos);
}

static const struct file_operations hostfs_file_fops = {
	.llseek		= generic_file_llseek,
	.read		= do_sync_read,
	.splice_read	= generic_file_splice_read,
	.aio_read	= generic_file_aio_read,
	.aio_write	= generic_file_aio_write,
	.write		= do_sync_write,
	.mmap		= generic_file_mmap,
	.open		= generic_file_open,
	.fsync		= upipe_file_fsync,
};

static const struct file_operations upipe_dir_fops = {
	.readdir	= upipe_readdir,
	.read		= generic_read_dir,
	.llseek		= default_llseek,
	.splice_read	= generic_file_splice_read,
};

static const struct address_space_operations upipe_aops = {
	.writepage = upipe_writepage,
	.readpage = upipe_readpage,
	.write_begin = upipe_write_begin,
	.write_end = upipe_write_end,
};


