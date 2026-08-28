This repository is developed inside a larger Perforce workspace even though the plugin itself is a Git repository.

Before modifying any existing file, first check whether the file is read-only.

If the file is read-only, run a Perforce checkout before editing it:

`p4 edit "<absolute file path>"`

After the Perforce checkout succeeds, continue with the normal file edit.

Do this only for files that are read-only. If a file is already writable, proceed normally.
