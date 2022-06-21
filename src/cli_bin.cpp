/*
 * Copyright (C) 2010 Vyatta, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <libgen.h>

#include <cli_cstore.h>
#include <cstore/cstore.hpp>
#include <cnode/cnode.hpp>
#include <commit/commit-algorithm.hpp>

using namespace cstore;

static int op_idx = -1;
static const char *op_bin_name[] = {
  "my_set",
  "my_delete",
  "my_activate",
  "my_deactivate",
  "my_rename",
  "my_copy",
  "my_comment",
  "my_discard",
  "my_move",
  "my_commit",
  NULL
};
static const char *op_Str[] = {
  "Set",
  "Delete",
  "Activate",
  "Deactivate",
  "Rename",
  "Copy",
  "Comment",
  "Discard",
  "Move",
  "Commit",
  NULL
};
static const char *op_str[] = {
  "set",
  "delete",
  "activate",
  "deactivate",
  "rename",
  "copy",
  "comment",
  "discard",
  "move",
  "commit",
  NULL
};
static const bool op_need_cfg_node_args[] = {
  true,
  true,
  true,
  true,
  true,
  true,
  true,
  false,
  true,
  false,
  false // dummy
};
static const bool op_use_edit_level[] = {
  true,
  true,
  true,
  true,
  true,
  true,
  true,
  true,
  true,
  false,
  false // dummy
};
#define OP_Str op_Str[op_idx]
#define OP_str op_str[op_idx]
#define OP_need_cfg_node_args op_need_cfg_node_args[op_idx]
#define OP_use_edit_level op_use_edit_level[op_idx]

static void
doSet(Cstore& cstore, const Cpath& path_comps)
{
  if (!cstore.validateSetPath(path_comps)) {
    bye("invalid set path\n");
  }
  if (!cstore.setCfgPath(path_comps)) {
    bye("set cfg path failed\n");
  }
}

static void
doDelete(Cstore& cstore, const Cpath& path_comps)
{
  if (!cstore.deleteCfgPath(path_comps)) {
    bye("delete failed\n");
  }
}

static void
doActivate(Cstore& cstore, const Cpath& path_comps)
{
  if (!cstore.validateActivatePath(path_comps)) {
    bye("%s validate failed", OP_str);
  }
  if (!cstore.unmarkCfgPathDeactivated(path_comps)) {
    bye("%s failed", OP_str);
  }
}

static void
doDeactivate(Cstore& cstore, const Cpath& path_comps)
{
  if (!cstore.validateDeactivatePath(path_comps)) {
    bye("%s validate failed", OP_str);
  }
  if (!cstore.markCfgPathDeactivated(path_comps)) {
    bye("%s failed", OP_str);
  }
}

static void
doRename(Cstore& cstore, const Cpath& path_comps)
{
  if (!cstore.validateRenameArgs(path_comps)) {
    bye("invalid rename args\n");
  }
  if (!cstore.renameCfgPath(path_comps)) {
    bye("rename cfg path failed\n");
  }
}

static void
doCopy(Cstore& cstore, const Cpath& path_comps)
{
  if (!cstore.validateCopyArgs(path_comps)) {
    bye("invalid copy args\n");
  }
  if (!cstore.copyCfgPath(path_comps)) {
    bye("copy cfg path failed\n");
  }
}

static void
doComment(Cstore& cstore, const Cpath& path_comps)
{
  if (!cstore.commentCfgPath(path_comps)) {
    bye("comment cfg path failed\n");
  }
}

static void
doDiscard(Cstore& cstore, const Cpath& args)
{
  if (args.size() > 0) {
    OUTPUT_USER("Invalid discard command\n");
    bye("invalid discard command\n");
  }
  if (!cstore.discardChanges()) {
    bye("discard failed\n");
  }
}

static void
doMove(Cstore& cstore, const Cpath& path_comps)
{
  if (!cstore.validateMoveArgs(path_comps)) {
    bye("invalid move args\n");
  }
  if (!cstore.moveCfgPath(path_comps)) {
    bye("move cfg path failed\n");
  }
}

static void
doCommit(Cstore& cstore, const Cpath& path_comps)
{
  Cpath dummy;
  cnode::CfgNode aroot(cstore, dummy, true, true);
  cnode::CfgNode wroot(cstore, dummy, false, true);
  if (!commit::doCommit(cstore, aroot, wroot)) {
    exit(1);
  }
}

typedef void (*OpFuncT)(Cstore& cstore,
                        const Cpath& path_comps);
OpFuncT OpFunc[] = {
  &doSet,
  &doDelete,
  &doActivate,
  &doDeactivate,
  &doRename,
  &doCopy,
  &doComment,
  &doDiscard,
  &doMove,
  &doCommit,
  NULL
};

static int
batch_set(Cstore& cstore, const std::vector<Cpath>& set_list)
{
  int error = 0;

  for (size_t i = 0; i < set_list.size(); i++) {
    if (!cstore.validateSetPath(set_list[i]) || !cstore.setCfgPath(set_list[i])) {
      error = 1;
      printf("Set '%s' failed\n", set_list[i].to_string().c_str());
    }
  }
  return error;
}

static int
batch_delete(Cstore& cstore, const std::vector<Cpath>& del_list)
{
  int error = 0;
  for (size_t i = 0; i < del_list.size(); i++) {
    if (!cstore.deleteCfgPath(del_list[i])) {
      error = 1;
      printf("Delete '%s' failed\n", del_list[i].to_string().c_str());
    }
  }
  return error;
}

int process_batch_commands(const char *op_name, const char *file_name)
{
  int error = 0;
  std::string cmd;
  std::vector<Cpath> commands;
  std::ifstream infile(file_name);

  Cstore *cstore = Cstore::createCstore(false);

  while (std::getline(infile, cmd)) {
    Cpath path;
    std::stringstream ss(cmd);
    for (std::string s; ss >> std::quoted(s, '\'');) {
      path.push(s);
    }
    commands.push_back(path);
  }

  if (strcmp(op_name, "my_batch_set") == 0)
    error = batch_set(*cstore, commands);
  if (strcmp(op_name, "my_batch_delete") == 0)
    error = batch_delete(*cstore, commands);

  delete cstore;

  if (error) {
    printf("Error in set/delete\n");
    return 1;
  }

  return 0;
}

int
main(int argc, char **argv)
{
  int i = 0;
  while (op_bin_name[i]) {
    if (strcmp(basename(argv[0]), op_bin_name[i]) == 0) {
      op_idx = i;
      break;
    }
    ++i;
  }
  if (op_idx == -1) {
    if ((strcmp(basename(argv[0]), "my_batch_set") != 0) &&
        (strcmp(basename(argv[0]), "my_batch_delete") != 0)) {
        printf("Invalid operation\n");
        exit(1);
    } else {
      int res = 0;
      if (argc < 2) {
        printf("batch set/delete requires a file argument\n");
        exit(1);
      }
      res = process_batch_commands(basename(argv[0]), argv[1]);
      if (res)
        exit(1);
      else
        exit(0);
    }
  }

  if (initialize_output(OP_Str) == -1) {
    bye("can't initialize output\n");
  }
  if (OP_need_cfg_node_args && argc < 2) {
    fprintf(out_stream, "Need to specify the config node to %s\n", OP_str);
    bye("nothing to %s\n", OP_str);
  }

  Cstore *cstore = Cstore::createCstore(OP_use_edit_level);
  Cpath path_comps(const_cast<const char **>(argv + 1), argc - 1);

  // call the op function
  OpFunc[op_idx](*cstore, path_comps);
  delete cstore;
  exit(0);
}

