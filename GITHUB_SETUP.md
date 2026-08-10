# GitHub repository setup

Suggested repository metadata:

- **Name:** `CPM22`
- **Description:** `CP/M 2.2 emulator and host-filesystem-backed Z80 environment with a reproducible CMake/zmac build.`
- **Topics:** `cpm`, `cpm22`, `z80`, `emulator`, `retrocomputing`, `cmake`, `zmac`

Recommended repository settings after the first push:

1. Enable Issues.
2. Enable private vulnerability reporting when available.
3. Require the `CMake CI` workflow before merging into the default branch.
4. Require pull requests for non-trivial changes.
5. Prevent force-pushes on the default branch.
6. Enable automatic deletion of merged branches.
7. Review `THIRD_PARTY.md` before making the repository public.

If `notwendig/z80` is private, create a repository secret named
`Z80_REPOSITORY_TOKEN` containing a fine-grained token with read access to that
repository. Public Z80 repositories can use the normal GitHub Actions token.
