/*******************************************************************
 * Copyright 2021-2080 evilbinary
 * 作者: evilbinary on 01/01/20
 * 邮箱: rootdebug@163.com
 ********************************************************************/

#include "vfs.h"

#include "fd.h"

vnode_t *root_node = NULL;

size_t vioctl(vnode_t *node, u32 cmd, void *args) {
  if (node->ioctl != NULL) {
    u32 ret = 0;
    // va_list args;
    // va_start(args, cmd);
    ret = node->ioctl(node, cmd, args);
    // va_end(args);
    return ret;
  } else {
    kprintf("node %s ioctl is null\n", node->name);
    return 0;
  }
}

u32 vread(vnode_t *node, u32 offset, u32 size, u8 *buffer) {
  if (node->read != NULL) {
    return node->read(node, offset, size, buffer);
  } else {
    kprintf("node %s read is null\n", node->name);
    return 0;
  }
}
u32 vwrite(vnode_t *node, u32 offset, u32 size, u8 *buffer) {
  if (node->write != NULL) {
    return node->write(node, offset, size, buffer);
  } else {
    kprintf("node %s write is null\n", node->name);
    return 0;
  }
}
u32 vopen(vnode_t *node) {
  if (node->open != NULL) {
    return node->open(node);
  } else {
    kprintf("node %s open is null \n", node->name);
    return;
  }
}
void vclose(vnode_t *node) {
  if (node->close != NULL) {
    return node->close(node);
  } else {
    kprintf("node %s close is null\n", node->name);
    return;
  }
}
u32 vreaddir(vnode_t *node, vdirent_t *dirent, u32 count) {
  if (node->readdir != NULL) {
    if ((node->flags & V_DIRECTORY) == V_DIRECTORY) {
      return node->readdir(node, dirent, count);
    } else {
      kprintf("node readdir is not dir\n");
    }
  } else {
    kprintf("node readdir is null\n");
    return 0;
  }
}
vnode_t *vfinddir(vnode_t *node, char *name) {
  if (node->finddir != NULL != NULL) {
    if ((node->flags & V_DIRECTORY) == V_DIRECTORY) {
      return node->finddir(node, name);
    } else {
      kprintf("node finddir is not dir\n");
    }
  } else {
    kprintf("node finddir is null\n");
    return 0;
  }
}

vnode_t *vfind(vnode_t *node, char *name) {
  if (node == NULL) {
    node = root_node;
  }
  if (node->find != NULL) {
    return node->find(node, name);
  } else {
    kprintf("node find is null\n");
    return 0;
  }
}

void vmount(vnode_t *root, u8 *path, vnode_t *node) {
  if (root->mount != NULL) {
    return node->mount(root, path, node);
  } else {
    kprintf("node mount is null\n");
    return;
  }
}

void vfs_exten_child(vnode_t *node) {
  u32 size = 4;
  if (node->child_number != 0) {
    size = node->child_number * 2;
  }
  vnode_t **child = kmalloc(size * sizeof(vnode_t *));
  for (int i = 0; i < size; i++) {
    child[i] = 0;
  }
  vnode_t **temp = node->child;
  if (node->child != NULL) {
    kmemmove(child, node->child, node->child_number * sizeof(vnode_t *));
    kfree(temp);
  }
  node->child = child;
  node->child_size = size;
}

void vfs_add_child(vnode_t *parent, vnode_t *child) {
  if ((parent->child_number + 1) > parent->child_size) {
    vfs_exten_child(parent);
  }
  if (parent->child == NULL) {
    kprintf("child alloc error\n");
    return;
  }
  child->parent = parent;
  parent->child[parent->child_number++] = child;
}

vnode_t *vfs_find(vnode_t *root, u8 *path) {
  char *token;
  const char *split = "/";
  char buf[256];
  char *start;
  char *s = buf;

  if (root == NULL) {
    root = root_node;
  }
  u32 path_len = kstrlen(path);
  if (path_len == 1 && kstrcmp(root->name, path) == 0) {
    return root;
  }
  if (path_len >= 256) {
    s = kmalloc(path_len);
    start = s;
  }
  kstrcpy(s, path);
  token = kstrtok(s, split);

  vnode_t *parent = root;
  vnode_t *node = NULL;
  if (token == NULL) {
    node = parent;
  }
  while (token != NULL) {
    // if (kstrcmp(token, parent->name) == 0) {
    //   continue;
    // }
    for (int i = 0; i < parent->child_number; i++) {
      vnode_t *n = parent->child[i];
      if (n == NULL) continue;
      if (kstrcmp(token, n->name) == 0) {
        node = n;
        parent = n;
        break;
      }
    }
    token = kstrtok(NULL, split);
  }
  if (path_len >= 256) {
    kfree(start);
  }
  // not found vfs vnode,is super block then find on block
  if (node == NULL && parent->super != NULL) {
    kstrcpy(s, path);
    token = kstrtok(s, split);
    if (kstrcmp(split, parent->name) == 0) {
    } else {
      while (token != NULL) {
        if (kstrcmp(token, parent->name) == 0) {
          break;
        }
        token = kstrtok(NULL, split);
      }
    }

    vnode_t *find_node = vfind(parent->super, token);
    if (find_node == NULL) {
      kprintf("open find %s failed \n", path);
      return NULL;
    }
    node = find_node;
    // mount to vfs
    vfs_add_child(parent, node);
  }

  return node;
}

void vfs_mount(vnode_t *root, u8 *path, vnode_t *node) {
  if (root == NULL) {
    root = root_node;
  }
  vnode_t *parent = vfs_find(root, path);
  if (parent != NULL) {
    vfs_add_child(parent, node);
  } else {
    kprintf("mount on %s error\n", path);
  }
}

u32 vfs_readdir(vnode_t *node, vdirent_t *dirent, u32 count) {
  // todo search int vfs
  if (node->super != NULL) {
    return node->super->readdir(node->super, dirent, count);
  }
  return 0;
}

u32 vfs_open(vnode_t *node) {
  int ret = 0;
  if (node->super != NULL) {
    vopen(node->super);
  }
  return ret;
}

vnode_t *vfs_create(u8 *name, u32 flags) {
  vnode_t *node = kmalloc(sizeof(vnode_t));
  node->name = kmalloc(kstrlen(name) + 1);
  kstrcpy(node->name, name);
  node->flags = flags;
  node->find = vfs_find;
  node->mount = vfs_mount;
  node->readdir = vfs_readdir;
  node->close = vfs_close;
  node->open = vfs_open;
  node->child = NULL;
  node->child_number = 0;
  node->child_size = 0;
  return node;
}

vnode_t *vfs_open_attr(vnode_t *root, u8 *name, u32 attr) {
  if (name == NULL) {
    return root;
  }
  if (root == NULL) {
    root = root_node;
  }
  vnode_t *node = vfs_find(root, name);
  vnode_t *file = node;
  if (file == NULL) {
    if (attr & O_CREAT == O_CREAT) {
      char *last = kstrrstr(name, root->name);
      if (last != NULL) {
        last += kstrlen(node->name);
        if (last[0] == '/') last++;
      }
      file = vfs_create(last, V_FILE);
      file->device = node->device;
      file->data = node->data;
      file->open = node->open;
    } else {
      kprintf("open second %s failed \n", name);
      return NULL;
    }
  }
  u32 ret = vfs_open(file);
  if (ret < 0) {
    kprintf("open third %s failed \n", name);
    return NULL;
  }
  return file;
}

void vfs_close(vnode_t *node) {
  if (node == NULL) {
    kprintf("close node is nul\n");
    return;
  }
  if (node->super != NULL) {
    vclose(node->super);
  }
}

int vfs_init() {
  root_node = vfs_create("/", V_DIRECTORY);
  return 1;
}