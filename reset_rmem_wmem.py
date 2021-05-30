import os

def set_procfs_values(rmem, values):
  """Writes TCP buffer sizes to procfs."""
  path = "/proc/sys/net/ipv4/tcp_" + ("rmem" if rmem else "wmem")
  #path = "tcp_" + ("rmem" if rmem else "wmem")

  if not os.access(path, os.W_OK):
    print("Error: not enough permissions to write to procfs")
    return False

  file = open(path, "w")
  file.write(values[0] + " " + values[1] + " " + values[2])

  return True

set_procfs_values(True, ["4096", "\t131072", "\t6291456"])
set_procfs_values(False, ["4096","\t16384", "\t4194304"])
