# irix-actions-runner examples

Example workflows for
[irix-actions-runner](https://github.com/sgidevnet/irix-actions-runner):

- `build.yml` builds figlet 2.2.5 on IRIX and uploads the binary and fonts.
- `parallel-hinv.yml` runs a 10-job matrix on virtualized Indys and collects
  their hardware inventories.

Both workflows run on a real SGI or a Linux-hosted pool under `runner serve`.

- [Successful figlet build](https://github.com/sgidevnet/irix-actions-figlet-demo/actions/runs/30654285492)
- [10-Indy matrix job](https://github.com/sgidevnet/irix-actions-figlet-demo/actions/runs/30564803634)

## Run

Requires irix-actions-runner 0.4.0 or later with the `irix` label. See the
[runner setup](https://github.com/sgidevnet/irix-actions-runner#getting-started).

```sh
gh workflow run build.yml
gh workflow run parallel-hinv.yml
```

## The figlet build failure

figlet enables TLF fonts with `XCFLAGS=-DTLF_FONTS`. Its `utf8.h` and
`zipio.h` use glibc's `__BEGIN_DECLS` and `__END_DECLS` macros, which IRIX does
not provide.

| Build | Result |
|---|---|
| Upstream defaults | Fails |
| `XCFLAGS=""` | Builds without TLF font support |
| Force-include `patches/irix-cdefs.h` | Builds with TLF font support |

The shim defines the missing macros and includes `<alloca.h>`, where IRIX
declares `alloca`. It is force-included through `CFLAGS`; upstream source is
unchanged.

The first diagnostic depends on the compiler:

```text
GCC 9.2:   utf8.h:30:1: error: unknown type name '__BEGIN_DECLS'
GCC 3.4.6: utf8.h:32: error: syntax error before "size_t"
```

The workflow requires the unpatched build to fail. If upstream begins to build
without the shim, the job fails.

## Runner constraints used by these workflows

| Constraint | Workflow behavior |
|---|---|
| `env:` values are not exported | Read `FIGLET_VERSION` and `FIGLET_FONT` through `${{ env.NAME }}` |
| Step `PATH` is fixed | Append `/usr/nekoware/bin:/usr/freeware/bin` in every step that needs it |
| Nekoware `curl` has no CA bundle | Pass `--cacert /usr/sgug/etc/pki/tls/certs/ca-bundle.crt` |
| Bash uses `-e -o pipefail` | Use `sed` or `awk` instead of pipelines ending in `head` |
| Hardware workspaces persist | Remove previous build and output directories before use |
| IRIX artifact uploads retain the leading directory | The hosted collector reads `out/...` inside each artifact |

The full SGUG-RSE install has GCC 9.2 at `/usr/sgug/bin/gcc`. The virtualized
guest has GCC 3.4.6 and `curl` under `/usr/nekoware/bin`. Appending the extra
paths keeps SGUG GCC first on real hardware.

## Files

| Path | Purpose |
|---|---|
| `.github/workflows/build.yml` | Fetch, fail unpatched, build with the shim, run and upload |
| `.github/workflows/parallel-hinv.yml` | Fan out 10 IRIX jobs and collect their output |
| `patches/irix-cdefs.h` | IRIX compatibility shim for figlet |
