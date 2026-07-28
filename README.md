# figlet on IRIX

Builds figlet 2.2.5 from upstream source on an SGI workstation under GitHub
Actions, as a worked example for
[irix-actions-runner](https://github.com/sgidevnet/irix-actions-runner). figlet
was chosen because it needs nothing beyond libc and builds in about twenty
seconds on a 400MHz R12000.

## Layout

    .github/workflows/build.yml   the workflow
    patches/irix-cdefs.h          the shim described below

## The bug

The Makefile sets `XCFLAGS` to `-DTLF_FONTS`, which pulls in `utf8.h`, which
opens with `__BEGIN_DECLS`. That macro belongs to glibc's `<sys/cdefs.h>` and
IRIX has no equivalent, so no declaration in the header parses. The first error
reported is in another file, several hundred lines from the cause.

    utf8.h:30:1: error: unknown type name '__BEGIN_DECLS'
    figlet.c:1149:5: warning: implicit declaration of function 'utf8_to_wchar'

Three ways it goes:

    make CC=gcc LD=gcc              fails, as upstream ships
    XCFLAGS=""                      builds, TLF font support off
    -include patches/irix-cdefs.h   builds, TLF fonts intact

Clearing `XCFLAGS` avoids the header rather than fixing it, and costs a
feature. The shim defines the two macros and is forced ahead of every
translation unit with `-include`, so no upstream file is edited.

The workflow builds unpatched first and requires that build to fail, so the
example stops being a demonstration the moment upstream or the toolchain
changes.

## Building it

Requires irix-actions-runner 0.3.0 or newer, registered with the `irix` label;
see
[irix-actions-runner](https://github.com/sgidevnet/irix-actions-runner#configure-and-run)
for setup.

    gh workflow run build.yml

## Runner constraints

These shape how the workflow is written. The runner's own README documents
them in full.

| Constraint | Write this instead |
|---|---|
| An `env:` block is not exported to the step's shell | Read it as `${{ env.NAME }}`, as this workflow does for `FIGLET_VERSION`. `$FIGLET_VERSION` is empty |
| No `hashFiles()`, and no `steps` context | Both fail the step by name. Without `$GITHUB_OUTPUT` there is nothing to put in `steps` |
| No `GITHUB_TOKEN` in the step environment | Put REST API work in a separate job on a hosted runner |
| Steps run under `-e`, bash steps under `-o pipefail` | Wrap a command you expect to fail in `if ...; then`, as the unpatched build is |
| Workspace is not wiped between jobs | Run `git clean -xdff` after checkout when a clean tree matters |

## Expressions

`${{ }}` is evaluated in a `run:` body, a `with:` value, an `if:` and a step
`name:`. Every context the job message carries is available, plus `env`,
`secrets` and a synthesised `runner`.

```yaml
- name: Fetch figlet ${{ env.FIGLET_VERSION }}
  run: |
    curl -fsSLo figlet.tar.gz \
      https://codeload.github.com/cmatsuoka/figlet/tar.gz/refs/tags/${{ env.FIGLET_VERSION }}

- name: Announce a tag build
  if: startsWith(github.ref, 'refs/tags/')
  run: ./figlet -w 72 RELEASE

- uses: actions/upload-artifact@v4
  with:
    name: figlet-irix-mips-n32-${{ github.run_number }}
```

That last one used to upload as `artifact`, because an expression in a `with:`
value read as absent.
