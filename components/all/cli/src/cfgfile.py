# Configuration file utilities

import fcntl

# Locks the specified file for exclusive access.
# Usage:
#   import filelock2
#   with filelock2.FileLock2('somefile') as f:
#     do_useful_things(f)

# WARNING: Since the file is opened in 'r+' mode, it must already exist!

class FileLock():

    def __init__(self, filename):
        self.filename = filename

    def __enter__(self):
        self.file = open(self.filename, 'r+')
        fcntl.flock(self.file, fcntl.LOCK_EX)
        return self.file

    def __exit__(self, type, value, traceback):
        fcntl.flock(self.file, fcntl.LOCK_UN)
        self.file.close()


# Returns a list of lines from the given file object.
# If normalize is True, extraneous whitespace (leading, internal, and trailing)
# will be removed.
# If joincontinuations is True, a line ending with a backslash will be joined
# with the following line. If the final line ends in a backslash, the 
# continuation will just stop without throwing an error.
def get_line_list_from_file(f, normalize=False, joincontinuations=False):
    lines = f.readlines()
    if normalize:
        lines = [' '.join(x.split()) for x in lines]
    if joincontinuations:
        it = iter(lines)
        lines = []
        for line in it:
            try:
                while len(line) > 0 and line[-1] == '\\':
                    line = line[:-1] + " " + it.next()
            except StopIteration:
                pass
            lines.append(line)
    return lines

# Writes the given list of lines to the specified file object,
# overwriting the file's existing contents.
# If addnewlines is True, the lines will be joined by newlines.
# Otherwise the lines will just be dumped.
def put_line_list_to_file(f, lines, addnewlines=False):
    f.seek(0)
    f.truncate(0)
    if addnewlines:
        f.write('\n'.join(lines))
    else:
        f.writelines(lines)

# Update the specified list lst with the value val.
# If remove is False:
# - val will be added to lst if it does not exist
# - AlreadyExistsError exception will be thrown if it does
# If remove is True:
# - val will be removed from lst if it exists
# - DoesNotExistError exception will be thrown if not
# returns True if lst was modified, False if not.
class AlreadyExistsError(Exception):
    pass
class DoesNotExistError(Exception):
    pass
def update_list(lst, val, remove=False):
    changed = False
    if remove:
        if val in lst:
            lst.remove(val)
            changed = True
        else:
            raise DoesNotExistError()
    else:
        if val not in lst:
            lst.append(val)
            lst.sort()
            changed = True
        else:
            raise AlreadyExistsError()
    return changed
