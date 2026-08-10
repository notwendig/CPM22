# Publishing checklist

Before making the repository public:

- [ ] Confirm the GitHub repository name/owner used by README badges and `CITATION.cff`.
- [ ] Confirm `notwendig/z80` is accessible to GitHub Actions, or configure the
      `Z80_REPOSITORY_TOKEN` secret for a private dependency.
- [ ] Review every item documented in `THIRD_PARTY.md` for redistribution rights.
- [ ] Run Debug and Release builds locally.
- [ ] Run `ctest --preset debug` and `ctest --preset release`.
- [ ] Check `git status` for generated files, core dumps, `.rej`, and accidental secrets.
- [ ] Push and verify the `CMake CI` workflow succeeds on GitHub.
