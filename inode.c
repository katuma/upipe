static int upipe_iget(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t dev, bool lookup)
{
	struct inode *inode = new_inode(sb);
	if (!inode)
		return NULL;

	PIPE_REQ;

	if (lookup) {
		OP(LOOKUP, dir);
	} else {
		OP(CREATE, dir);
		PUT32(mode);
		PUT32(dev);
	}

	DENTRY(dentry);

	if (!SUBMIT() || req.remain != 6*4) {
		drop_inode(inode);
		return req.errno;
	}

	uint32_t uid, gid, nlink, rdev, mode;

	GET32(mode);
	GET32(uid);
	GET32(gid);
	GET32(nlink);
	GET32(rdev);

	GET32(inode->i_atime);
	GET32(inode->i_mtime);
	GET32(inode->i_ctime);
	GET32(inode->i_size);
	GET32(inode->i_blocks);
	GET32(inode->i_ino);
	GET32(inode->i_private);

	switch (inode->i_mode & S_IFMT) {
		case S_IFLNK:
			inode->i_op = &upipe_iops;
			break;
		case S_IFDIR:
			inode->i_op = &upipe_iops;
			inode->i_fop = &upipe_dir_fops;
			break;
		case S_IFCHR:
		case S_IFBLK:
		case S_IFIFO:
		case S_IFSOCK:
			init_special_inode(inode, inode->i_mode & S_IFMT, rdev);
			break;
		default:
			inode->i_op = &upipe_iops;
			inode->i_fop = &upipe_file_fops;
			inode->i_mapping->a_ops = &upipe_aops;
			break;
	}

	inode->i_mode = mode;
	set_nlink(inode, nlink);
	i_uid_write(inode, uid);
	i_gid_write(inode, gid);

	if (!lookup)
		d_instantiate(dentry, inode);
	else
		d_add(dentry, inode);
	return 0;
}

static int upipe_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t dev)
{
	return upipe_iget(dir, dentry, mode, dev, false);
}

static int upipe_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	return upipe_mknod(dir, dentry, mode | S_IFDIR, 0);
}

static int upipe_create(struct inode *dir, struct dentry *dentry, umode_t mode, bool excl)
{
	return upipe_mknod(dir, dentry, mode | S_IFREG, 0);
}

static struct dentry *upipe_lookup(struct inode *ino, struct dentry *dentry, unsigned int flags)
{
	return ERR_PTR(upipe_iget(dir, dentry, 0, 0, true));
}

static int upipe_link(struct dentry *newname, struct inode *newnamedir, struct dentry *src)
{
	PIPE_REQ;

	OP(LINK, src->parent->d_ino);
	INODE(newnamedir);
	DENTRY(src);
	DENTRY(newname);

	SUBMIT_REQ;
}

static int upipe_unlink(struct inode *where, struct dentry *dentry)
{
	PIPE_REQ;

	OP(UNLINK, where);

	DENTRY(dentry);

	SUBMIT_REQ;
}

static int upipe_symlink(struct inode *newnamedir, struct dentry *newname, const char *contents)
{
	PIPE_REQ;

	OP(SYMLINK, newnamedir);

	DENTRY(newname);
	ASCIIZ(contents);

	SUBMIT_REQ;
}

static int upipe_rename(struct inode *fromdir, struct dentry *froment, struct inode *todir, struct dentry *toent)
{
	PIPE_REQ;

	OP(RENAME, fromdir);

	INODE(todir);
	DENTRY(froment);
	DENTRY(toent);

	SUBMIT_REQ;
}

static void *upipe_follow_link(struct dentry *dentry, struct nameidata *nd) {
	PIPE_REQ;

	OP(READLINK, dentry->parent);
	DENTRY(dentry);

	if (!SUBMIT() || req.remain < 1)
		return ERR_PTR(req.errno);

	buf = kzalloc(buf.remain, GFP_KERNEL);
	memcpy(buf, req.p, req.remain);
	buf[req.remain-1] = 0;

	nd_set_link(nd, buf);
	return NULL;
}

static void upipe_put_link(struct dentry *dentry, struct nameidata *nd, void *ptr)
{
	char *buf = nd_get_link(nd);
	if (!IS_ERR(buf))
		kfree(buf);
}

static const struct inode_operations upipe_iops = {
	.create		= upipe_create,
	.lookup		= upipe_lookup,
	.link		= upipe_link,
	.unlink		= upipe_unlink,
	.symlink	= upipe_symlink,
	.mkdir		= upipe_mkdir,
	.rmdir		= upipe_rmdir,
	.mknod		= upipe_mknod,
	.rename		= upipe_rename,
        .readlink	= generic_readlink,
        .follow_link	= upipe_follow_link,
        .put_link       = upipe_put_link
};


