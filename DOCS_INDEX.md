# Documentation Index

## Main Documentation

📚 **[DOCUMENTATION.md](DOCUMENTATION.md)** - Complete comprehensive guide (2073 lines, ~46 KB)

This single file contains everything you need:
- Installation and building
- Quick start examples
- Complete command reference
- Topography guide with all terrain types
- Bottom friction guide with coefficient tables
- Example scenarios for various applications
- Output and visualization tutorials
- Performance optimization and parallelization
- Troubleshooting guide
- Mathematical formulations
- Full references

## Quick Reference

📖 **[README.md](README.md)** - Project overview and quick start

## CI/CD Pipeline

🔧 **[.github/CI.md](.github/CI.md)** - GitHub Actions pipeline documentation
- Automated linting, testing, and builds
- Code coverage analysis
- Artifact publishing
- Pull request automation

## Example Script

### Example Suite
- **run_examples.sh** - Comprehensive example script with 12 scenarios + parallelization benchmark
   - Basic flow patterns
   - Friction effects (Manning, Chezy)
   - Topography interactions
   - Realistic applications
   - Performance benchmark (single vs multi-threaded)

## Getting Started

1. **Install:** See DOCUMENTATION.md → Installation & Building
2. **Quick Test:**
   ```bash
   cargo run --release -- --nx 20 --ny 20 --final-time 1.0
   ```
3. **Learn:** Read DOCUMENTATION.md sections 1-6
4. **Explore:** Try example scenarios in DOCUMENTATION.md section 9
5. **Visualize:** Follow ParaView guide in DOCUMENTATION.md section 10

## Need Help?

- **Compilation issues:** See DOCUMENTATION.md → Troubleshooting → Compilation Errors
- **Runtime problems:** See DOCUMENTATION.md → Troubleshooting → Common Issues
- **Command syntax:** See DOCUMENTATION.md → Command Reference
- **Physical setup:** See DOCUMENTATION.md → Topography Guide / Friction Guide

All answers are in DOCUMENTATION.md!
