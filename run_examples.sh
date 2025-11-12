#!/bin/bash
# Comprehensive Test Suite for Shallow Water Solver
# Includes basic tests and extended tests with topography and friction

# Color codes for better output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to run a test
run_test() {
    local test_num=$1
    local test_name=$2
    shift 2
    echo ""
    echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}Test ${test_num}: ${test_name}${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
    cargo run --release -- "$@"
    echo -e "${GREEN}✓ Test ${test_num} completed${NC}"
}

# Header
clear
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                                                               ║"
echo "║        Shallow Water Solver - Comprehensive Test Suite       ║"
echo "║                                                               ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""
echo "This script will run all test scenarios including:"
echo "  • Basic flow patterns (dam break, waves)"
echo "  • Topography effects (slope, hill, channel)"
echo "  • Bottom friction (Manning, Chezy)"
echo "  • Comparison studies"
echo "  • Parallelization benchmark"
echo ""
echo -e "${YELLOW}Estimated total runtime: ~5-10 minutes${NC}"
echo ""
read -p "Press Enter to start tests (or Ctrl+C to cancel)..."

# ============================================================================
# SECTION 1: BASIC TESTS (No topography, no friction)
# ============================================================================
echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  SECTION 1: Basic Flow Patterns                               ║"
echo "╚═══════════════════════════════════════════════════════════════╝"

run_test "1.1" "Dam Break (Classic)" \
  --nx 40 --ny 40 \
  --final-time 2.0 \
  --initial-condition dam-break \
  --output-prefix test1_dambreak \
  --output-interval 0.2

run_test "1.2" "Circular Wave" \
  --nx 40 --ny 40 \
  --final-time 3.0 \
  --initial-condition circular-wave \
  --output-prefix test2_wave \
  --output-interval 0.2

run_test "1.3" "Standing Wave" \
  --nx 40 --ny 40 \
  --final-time 1.5 \
  --initial-condition standing-wave \
  --output-prefix test3_standing \
  --output-interval 0.15

# ============================================================================
# SECTION 2: FRICTION TESTS
# ============================================================================
echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  SECTION 2: Bottom Friction Effects                           ║"
echo "╚═══════════════════════════════════════════════════════════════╝"

run_test "2.1" "Dam Break with Manning Friction (n=0.03)" \
  --nx 40 --ny 40 \
  --final-time 2.0 \
  --initial-condition dam-break \
  --friction manning --manning-n 0.03 \
  --output-prefix test4_dam_friction \
  --output-interval 0.2

run_test "2.2" "Wave WITHOUT Friction (Reference)" \
  --nx 40 --ny 40 \
  --final-time 3.0 \
  --initial-condition circular-wave \
  --friction none \
  --output-prefix test5a_no_friction \
  --output-interval 0.3

run_test "2.3" "Wave WITH Friction (Comparison)" \
  --nx 40 --ny 40 \
  --final-time 3.0 \
  --initial-condition circular-wave \
  --friction manning --manning-n 0.05 \
  --output-prefix test5b_with_friction \
  --output-interval 0.3

run_test "2.4" "Dam Break with Chezy Friction (C=40)" \
  --nx 40 --ny 40 \
  --final-time 2.0 \
  --initial-condition dam-break \
  --friction chezy --chezy-c 40.0 \
  --output-prefix test6_chezy \
  --output-interval 0.2

# ============================================================================
# SECTION 3: TOPOGRAPHY TESTS
# ============================================================================
echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  SECTION 3: Topography and Terrain Effects                    ║"
echo "╚═══════════════════════════════════════════════════════════════╝"

run_test "3.1" "Circular Wave Over Gaussian Hill" \
  --nx 50 --ny 50 \
  --final-time 2.0 \
  --initial-condition circular-wave \
  --topography gaussian \
  --friction manning --manning-n 0.02 \
  --output-prefix test7_wave_hill \
  --output-interval 0.2

run_test "3.2" "Dam Break on Sloped Bed" \
  --nx 40 --ny 40 \
  --final-time 3.0 \
  --initial-condition dam-break \
  --topography slope \
  --friction manning --manning-n 0.025 \
  --output-prefix test8_slope_dam \
  --output-interval 0.3

run_test "3.3" "Standing Wave in Channel" \
  --nx 40 --ny 40 \
  --final-time 2.0 \
  --initial-condition standing-wave \
  --topography channel \
  --friction manning --manning-n 0.035 \
  --output-prefix test9_channel \
  --output-interval 0.2

# ============================================================================
# SECTION 4: APPLICATION SCENARIOS
# ============================================================================
echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  SECTION 4: Realistic Application Scenarios                   ║"
echo "╚═══════════════════════════════════════════════════════════════╝"

run_test "4.1" "Urban Flood (Smooth Surface, Slope)" \
  --nx 60 --ny 60 \
  --width 50.0 --height 50.0 \
  --final-time 5.0 \
  --initial-condition dam-break \
  --topography slope \
  --friction manning --manning-n 0.015 \
  --output-prefix test10_urban_flood \
  --output-interval 0.5

run_test "4.2" "Vegetated Floodplain (High Friction)" \
  --nx 40 --ny 40 \
  --final-time 5.0 \
  --initial-condition dam-break \
  --friction manning --manning-n 0.08 \
  --output-prefix test11_vegetation \
  --output-interval 0.5

run_test "4.3" "River Channel Flow" \
  --nx 50 --ny 50 \
  --width 30.0 --height 30.0 \
  --final-time 3.0 \
  --initial-condition standing-wave \
  --topography channel \
  --friction manning --manning-n 0.03 \
  --output-prefix test12_river \
  --output-interval 0.3

# ============================================================================
# SECTION 5: PARALLELIZATION BENCHMARK
# ============================================================================
echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║  SECTION 5: Parallelization Performance Benchmark             ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""
echo "This benchmark compares single-threaded vs multi-threaded performance"
echo "on a medium-sized mesh (80×80 = 12,482 triangles)."
echo ""
read -p "Press Enter to run benchmark (or Ctrl+C to skip)..."

# Test configuration
NX=80
NY=80
FINAL_TIME=1.0
INITIAL_CONDITION="dam-break"

echo ""
echo "Benchmark Configuration:"
echo "  Mesh: ${NX}×${NY} ($(( (NX-1)*(NY-1)*2 )) triangles)"
echo "  Final time: ${FINAL_TIME}s"
echo "  Initial condition: ${INITIAL_CONDITION}"
echo ""

# Single-threaded test
echo "─────────────────────────────────────────────────────────────"
echo "Benchmark 1: Single-threaded (RAYON_NUM_THREADS=1)"
echo "─────────────────────────────────────────────────────────────"
BENCH1_START=$(date +%s.%N)
RAYON_NUM_THREADS=1 cargo run --release -- \
  --nx $NX --ny $NY \
  --final-time $FINAL_TIME \
  --initial-condition $INITIAL_CONDITION \
  --output-prefix bench_1thread \
  --output-interval 100.0 2>&1 | grep -E "Total steps"
BENCH1_END=$(date +%s.%N)
BENCH1_TIME=$(echo "$BENCH1_END - $BENCH1_START" | bc)
echo "Time: ${BENCH1_TIME}s"
echo ""

# Multi-threaded test (all cores)
echo "─────────────────────────────────────────────────────────────"
echo "Benchmark 2: Multi-threaded (all cores)"
echo "─────────────────────────────────────────────────────────────"
BENCH2_START=$(date +%s.%N)
cargo run --release -- \
  --nx $NX --ny $NY \
  --final-time $FINAL_TIME \
  --initial-condition $INITIAL_CONDITION \
  --output-prefix bench_multithread \
  --output-interval 100.0 2>&1 | grep -E "Total steps"
BENCH2_END=$(date +%s.%N)
BENCH2_TIME=$(echo "$BENCH2_END - $BENCH2_START" | bc)
echo "Time: ${BENCH2_TIME}s"
echo ""

# Calculate speedup
SPEEDUP=$(echo "scale=2; $BENCH1_TIME / $BENCH2_TIME" | bc)

# Get CPU info
CPU_CORES=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 'unknown')

echo "─────────────────────────────────────────────────────────────"
echo -e "${GREEN}Benchmark Results:${NC}"
echo "  Single-threaded time: ${BENCH1_TIME}s"
echo "  Multi-threaded time:  ${BENCH2_TIME}s"
echo -e "  ${YELLOW}Speedup: ${SPEEDUP}x${NC}"
echo "  CPU cores: ${CPU_CORES}"
echo "─────────────────────────────────────────────────────────────"
echo ""

# ============================================================================
# SUMMARY
# ============================================================================
echo ""
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║                   ALL TESTS COMPLETED!                        ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""
echo -e "${GREEN}✓ All 12 test scenarios + parallelization benchmark completed${NC}"
echo ""
echo "Output Files Generated:"
echo "──────────────────────────────────────────────────────────────"
echo "  Basic Tests:"
echo "    test1_dambreak_*.vtk      - Classic dam break"
echo "    test2_wave_*.vtk          - Circular wave"
echo "    test3_standing_*.vtk      - Standing wave"
echo ""
echo "  Friction Tests:"
echo "    test4_dam_friction_*.vtk  - Dam break with Manning"
echo "    test5a_no_friction_*.vtk  - Wave without friction"
echo "    test5b_with_friction_*.vtk - Wave with friction"
echo "    test6_chezy_*.vtk         - Dam break with Chezy"
echo ""
echo "  Topography Tests:"
echo "    test7_wave_hill_*.vtk     - Wave over Gaussian hill"
echo "    test8_slope_dam_*.vtk     - Dam break on slope"
echo "    test9_channel_*.vtk       - Flow in channel"
echo ""
echo "  Application Scenarios:"
echo "    test10_urban_flood_*.vtk  - Urban flood simulation"
echo "    test11_vegetation_*.vtk   - Vegetated floodplain"
echo "    test12_river_*.vtk        - River channel flow"
echo ""
echo "Visualization Instructions:"
echo "──────────────────────────────────────────────────────────────"
echo "  1. Install ParaView: https://www.paraview.org/"
echo "  2. Open ParaView"
echo "  3. File → Open → Select test*.vtk files"
echo "  4. Click 'Apply' in Properties panel"
echo "  5. Select variable from dropdown:"
echo "       • 'height' - Water depth"
echo "       • 'water_surface' - Free surface elevation"
echo "       • 'velocity' - Flow velocity"
echo "       • 'bed_elevation' - Bottom topography"
echo "  6. Optional: Add 'Warp By Scalar' filter for 3D view"
echo "  7. Press Play button to animate"
echo ""
echo "Comparison Studies:"
echo "──────────────────────────────────────────────────────────────"
echo "  • Friction effect: Compare test5a vs test5b"
echo "  • Friction laws: Compare test4 (Manning) vs test6 (Chezy)"
echo "  • Topography: Compare test1 vs test8 (slope effect)"
echo "  • Applications: Examine test10, test11, test12"
echo "  • Performance: Parallel speedup demonstrated in benchmark"
echo ""
echo "Next Steps:"
echo "──────────────────────────────────────────────────────────────"
echo "  • Review DOCUMENTATION.md for detailed analysis"
echo "  • Review PARALLELIZATION.md for performance optimization"
echo "  • Modify parameters in this script for custom tests"
echo "  • Run individual tests with: cargo run --release -- [options]"
echo "  • Check mass conservation errors in output logs"
echo "  • Control threads with: RAYON_NUM_THREADS=N cargo run ..."
echo ""
echo -e "${BLUE}Happy computing! 🌊${NC}"
echo ""
