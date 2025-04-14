format:
	find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.cc" -o -name "*.hpp" -o -name "*.cxx" \) -exec clang-format -i {} +