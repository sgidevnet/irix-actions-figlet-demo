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
IRIX has no equivalent, so no declaration in the header parses. `zipio.h` opens
the same way.

How legible that is depends on the compiler. GCC 9.2 names the macro, and the
first thing it reports is a symptom several hundred lines away:

    utf8.h:30:1: error: unknown type name '__BEGIN_DECLS'
    figlet.c:1149:5: warning: implicit declaration of function 'utf8_to_wchar'

GCC 3.4.6, which is what the emulated worker image has, never mentions it. It
points at the declaration that followed instead, with no column and no note of
what came before:

    utf8.h:32: error: syntax error before "size_t"
    zipio.h:76: error: syntax error before "typedef"

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

Requires irix-actions-runner 0.4.0 or newer, registered with the `irix` label;
see
[irix-actions-runner](https://github.com/sgidevnet/irix-actions-runner#getting-started)
for setup. A real SGI and an emulated pool under `runner serve` both work.

    gh workflow run build.yml

## What the guest ships

A full SGUG-RSE install puts GCC 9.2 at `/usr/sgug/bin/gcc`, which is on the
step PATH. The published emulated worker image carries an SGUG-RSE subset
(`git`, `bash`, `zip`, `unzip`) alongside Nekoware, so `gcc` there is 3.4.6 at
`/usr/nekoware/bin/gcc` and `curl` is `/usr/nekoware/bin/curl`. Neither
directory is on the step PATH, which is why the build steps extend it
themselves. MIPSPro's `cc` and `c99` are under `/usr/bin` either way.

That Nekoware `curl` was built with no default CA bundle: `curl-config --ca` is
empty and every https URL ends at exit 60, `unable to get local issuer
certificate`. SGUG-RSE ships a bundle at
`/usr/sgug/etc/pki/tls/certs/ca-bundle.crt`, so the fetch step passes
`--cacert`, which is correct on both kinds of machine.

The `Environment` step prints `command -v gcc` and the version line, so the job
log names the compiler that built the binary.

## Runner constraints

These shape how the workflow is written. The runner's own README documents
them in full.

| Constraint | Write this instead |
|---|---|
| An `env:` block is not exported to the step's shell | Read it as `${{ env.NAME }}`, as this workflow does for `FIGLET_VERSION`. `$FIGLET_VERSION` is empty |
| The step PATH is fixed at `/usr/sgug/bin:/usr/sgug/sbin:/usr/bin:/bin:/usr/sbin:/usr/bsd` | Nekoware and Freeware are not on it, so a step needing either appends to `PATH` itself. Nothing carries the export forward, so every such step repeats it |
| No `hashFiles()`, and no `steps` context | Both fail the step by name. Without `$GITHUB_OUTPUT` there is nothing to put in `steps` |
| No `GITHUB_TOKEN` in the step environment | Put REST API work in a separate job on a hosted runner |
| Steps run under `-e`, bash steps under `-o pipefail` | Wrap a command you expect to fail in `if ...; then`, as the unpatched build is. `cmd \| head -3` also fails the step, because the producer dies of SIGPIPE once `head` has its lines. Use `sed -n 1,3p` |
| Workspace is not wiped between jobs | Run `git clean -xdff` after checkout when a clean tree matters |

## Expressions

`${{ }}` is evaluated in a `run:` body, a `with:` value, an `if:` and a step
`name:`. Every context the job message carries is available, plus `env`,
`secrets` and a synthesised `runner`.

```yaml
- name: Fetch figlet ${{ env.FIGLET_VERSION }}
  run: |
    curl -fsSLo figlet.tar.gz \
      --cacert /usr/sgug/etc/pki/tls/certs/ca-bundle.crt \
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
