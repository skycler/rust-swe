# GitHub Actions CI Pipeline

This project uses a comprehensive GitHub Actions workflow for continuous integration and quality assurance.

## Pipeline Overview

The CI pipeline (`.github/workflows/ci.yml`) runs automatically on:
- All pushes to `main` and `develop` branches
- All pull requests targeting `main` and `develop` branches

## Pipeline Jobs

### 1. **Lint** (Clippy)
- **Container**: `rust:1.83`
- **Purpose**: Static analysis and code quality checks
- **Command**: `cargo clippy --all-targets --all-features -- -D warnings`
- **Output**: Clippy results uploaded as artifacts

### 2. **Format Check** (rustfmt)
- **Container**: `rust:1.83`
- **Purpose**: Ensures consistent code formatting
- **Command**: `cargo fmt -- --check`
- **Fails if**: Code is not properly formatted

### 3. **Unit Tests**
- **Container**: `rust:1.83`
- **Purpose**: Runs all 22 unit tests
- **Commands**:
  - `cargo test --verbose -- --nocapture`
  - `cargo test --verbose -- --format=json` (for CI tools)
- **Output**: 
  - Test results in JSON format
  - Human-readable test report (Markdown)
  - Both uploaded as artifacts

### 4. **Build**
- **Container**: `rust:1.83`
- **Strategy**: Matrix build (debug + release)
- **Purpose**: Verifies compilation in both modes
- **Commands**:
  - `cargo build --verbose` (debug)
  - `cargo build --release --verbose` (release)
- **Output**: Compiled binaries uploaded as artifacts
  - `shallow-water-solver-debug`
  - `shallow-water-solver-release`

### 5. **Coverage**
- **Container**: `rust:1.83`
- **Purpose**: Code coverage analysis
- **Tool**: `cargo-tarpaulin`
- **Command**: `cargo tarpaulin --verbose --all-features --workspace --timeout 120 --out xml`
- **Output**: 
  - Coverage report uploaded to Codecov
  - XML report as artifact

### 6. **Publish Results**
- **Container**: Ubuntu latest (no Rust needed)
- **Purpose**: Aggregate and publish all results
- **Actions**:
  - Downloads all artifacts
  - Creates comprehensive summary report
  - Publishes to GitHub job summary
  - Comments on pull requests with results
- **Runs**: Always (even if previous jobs fail)

## Artifacts

All jobs produce artifacts that are retained for 30 days:

| Artifact Name | Contents | Job |
|---------------|----------|-----|
| `clippy-results` | Linter output | Lint |
| `test-results` | JSON test results + Markdown report | Test |
| `shallow-water-solver-debug` | Debug binary | Build |
| `shallow-water-solver-release` | Release binary | Build |
| `coverage-report` | Code coverage XML | Coverage |
| `ci-summary` | Overall pipeline summary | Publish |

## Viewing Results

### In GitHub UI
1. Go to repository → **Actions** tab
2. Click on any workflow run
3. View job results and download artifacts

### In Pull Requests
- CI status appears as checks on PR
- Automated comment with summary is posted
- Click "Details" to see full logs

### Badges
The README includes a CI badge showing pipeline status:
```markdown
[![CI Pipeline](https://github.com/skycler/rust-swe/actions/workflows/ci.yml/badge.svg)](https://github.com/skycler/rust-swe/actions/workflows/ci.yml)
```

## Performance

All jobs use caching to speed up builds:
- Cargo registry cache
- Cargo index cache
- Build target cache

Typical run times:
- **Lint**: ~2-3 minutes
- **Format**: ~1 minute
- **Test**: ~2-3 minutes
- **Build** (per profile): ~3-5 minutes
- **Coverage**: ~5-8 minutes
- **Total**: ~15-20 minutes

## Local Testing

Run the same checks locally before pushing:

```bash
# Format check
cargo fmt -- --check

# Lint
cargo clippy --all-targets --all-features -- -D warnings

# Test
cargo test --verbose

# Build both profiles
cargo build
cargo build --release

# Coverage (requires installation)
cargo install cargo-tarpaulin
cargo tarpaulin --verbose --all-features --workspace
```

## Troubleshooting

### Failed Clippy
- Run locally: `cargo clippy --all-targets --all-features -- -D warnings`
- Fix all warnings (the CI treats warnings as errors)

### Failed Format Check
- Run: `cargo fmt`
- Commit the formatted code

### Failed Tests
- Run locally: `cargo test --verbose -- --nocapture`
- Check test output for specific failures
- All 22 tests must pass

### Failed Build
- Usually indicates compilation errors
- Run: `cargo build --verbose` to see detailed errors

### Slow CI
- Caches may have been invalidated
- Large dependencies take time to compile
- First run after Cargo.lock change is slower

## Extending the Pipeline

To add new checks or jobs:

1. Edit `.github/workflows/ci.yml`
2. Add new job following existing pattern:
```yaml
new-job:
  name: New Check
  runs-on: ubuntu-latest
  container:
    image: rust:1.83
  steps:
    - uses: actions/checkout@v4
    - run: cargo your-command
```
3. Add to `needs:` in `publish-results` job
4. Test with a pull request

## Security

- All jobs run in isolated containers
- No secrets are required for basic CI
- Codecov integration uses public uploads (no token needed for public repos)
- Artifacts are only accessible to repository members

## References

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Rust Docker Images](https://hub.docker.com/_/rust)
- [cargo-tarpaulin](https://github.com/xd009642/tarpaulin)
- [Codecov GitHub Action](https://github.com/codecov/codecov-action)
