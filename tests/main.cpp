//
// Created by mattfor on 8/16/26.
//

#include <print>
#include <cstdlib>
#include <string_view>
#include "helpers/TestRunner.h"

namespace SettingsTests
{
void read_value();
void load_valid_config();
void get_loaded_manager();
void reject_missing_config();
void reject_duplicate_config();
void missing_manager_returns_null();
} // namespace SettingsTests

namespace HashingTests
{
void one_byte_correct_hash();
void dummy_file_correct_hash();
} // namespace HashingTests

namespace MemoryMapTests
{
void find_known_address();
void resolve_own_image();
void regions_are_sorted();
void describe_protection();
void filter_by_protection();
void covers_contiguous_range();
void parse_platform_region();
void reject_invalid_process();
void refresh_current_process();
void filter_by_address_window();
void reject_unmapped_address();
} // namespace MemoryMapTests

namespace MemoryScannerTests
{
void read_buffer();
void read_string();
void read_struct();
void read_integer();
void read_zero_bytes();
void read_byte_span();
void scan_finds_marker();
void read_single_byte();
void read_typed_value();
void write_typed_value();
void map_isolates_region();
void reject_null_buffer();
void scan_respects_alignment();
void attach_current_process();
void reject_invalid_address();
void reject_invalid_process();
void scan_finds_typed_value();
void reports_running_process();
void detach_releases_process();
void pattern_parses_wildcards();
void refine_narrows_candidates();
void scan_respects_match_limit();
void read_null_terminated_string();
void scan_signature_with_wildcards();
void attach_current_process_twice();
void pattern_matches_nibble_wildcard();
void snapshot_detects_content_change();
void snapshot_detects_unmapped_region();
void snapshot_accepts_untouched_region();
void snapshot_detects_protection_change();
void pattern_rejects_malformed_signature();
void read_does_not_overwrite_beyond_size();
} // namespace MemoryScannerTests

int main()
{
	constexpr Test tests_settings[] = {
		{ .name     = "Settings::loadConfig valid config",
		  .function = SettingsTests::load_valid_config },
		{ .name     = "Settings::getIniManager loaded config",
		  .function = SettingsTests::get_loaded_manager },
		{ .name     = "Settings::read value",
		  .function = SettingsTests::read_value },
		{ .name     = "Settings::reject missing config",
		  .function = SettingsTests::reject_missing_config },
		{ .name     = "Settings::reject duplicate config",
		  .function = SettingsTests::reject_duplicate_config },
		{ .name     = "Settings::missing manager returns null",
		  .function = SettingsTests::missing_manager_returns_null },
	};

	constexpr Test tests_hashing[] = {
		{ .name     = "Hashing::one byte returns correct hash",
		  .function = HashingTests::one_byte_correct_hash },
		{ .name     = "Hashing::dummy file returns correct hash",
		  .function = HashingTests::dummy_file_correct_hash }
	};

	constexpr Test tests_memory_scanner[] = {
		{ .name     = "MemoryScanner::attach current process",
		  .function = MemoryScannerTests::attach_current_process },
		{ .name     = "MemoryScanner::attach current process twice",
		  .function = MemoryScannerTests::attach_current_process_twice },
		{ .name     = "MemoryScanner::read integer",
		  .function = MemoryScannerTests::read_integer },
		{ .name     = "MemoryScanner::read single byte",
		  .function = MemoryScannerTests::read_single_byte },
		{ .name     = "MemoryScanner::read buffer",
		  .function = MemoryScannerTests::read_buffer },
		{ .name     = "MemoryScanner::read string",
		  .function = MemoryScannerTests::read_string },
		{ .name     = "MemoryScanner::read struct",
		  .function = MemoryScannerTests::read_struct },
		{ .name     = "MemoryScanner::read does not overwrite beyond size",
		  .function = MemoryScannerTests::read_does_not_overwrite_beyond_size },
		{ .name     = "MemoryScanner::read invalid address",
		  .function = MemoryScannerTests::reject_invalid_address },
		{ .name     = "MemoryScanner::read null buffer",
		  .function = MemoryScannerTests::reject_null_buffer },
		{ .name     = "MemoryScanner::read zero bytes",
		  .function = MemoryScannerTests::read_zero_bytes },
		{ .name     = "MemoryScanner::detach releases process",
		  .function = MemoryScannerTests::detach_releases_process },
		{ .name     = "MemoryScanner::reject invalid process",
		  .function = MemoryScannerTests::reject_invalid_process },
		{ .name     = "MemoryScanner::report running process",
		  .function = MemoryScannerTests::reports_running_process },
		{ .name     = "MemoryScanner::read typed value",
		  .function = MemoryScannerTests::read_typed_value },
		{ .name     = "MemoryScanner::read byte span",
		  .function = MemoryScannerTests::read_byte_span },
		{ .name     = "MemoryScanner::read null terminated string",
		  .function = MemoryScannerTests::read_null_terminated_string },
		{ .name     = "MemoryScanner::write typed value",
		  .function = MemoryScannerTests::write_typed_value },
		{ .name     = "MemoryScanner::pattern parses wildcards",
		  .function = MemoryScannerTests::pattern_parses_wildcards },
		{ .name     = "MemoryScanner::pattern matches nibble wildcard",
		  .function = MemoryScannerTests::pattern_matches_nibble_wildcard },
		{ .name     = "MemoryScanner::pattern rejects malformed signature",
		  .function = MemoryScannerTests::pattern_rejects_malformed_signature },
		{ .name     = "MemoryScanner::scan finds marker",
		  .function = MemoryScannerTests::scan_finds_marker },
		{ .name     = "MemoryScanner::scan signature with wildcards",
		  .function = MemoryScannerTests::scan_signature_with_wildcards },
		{ .name     = "MemoryScanner::scan respects alignment",
		  .function = MemoryScannerTests::scan_respects_alignment },
		{ .name     = "MemoryScanner::scan respects match limit",
		  .function = MemoryScannerTests::scan_respects_match_limit },
		{ .name     = "MemoryScanner::scan finds typed value",
		  .function = MemoryScannerTests::scan_finds_typed_value },
		{ .name     = "MemoryScanner::refine narrows candidates",
		  .function = MemoryScannerTests::refine_narrows_candidates },
		{ .name     = "MemoryScanner::map isolates region",
		  .function = MemoryScannerTests::map_isolates_region },
		{ .name     = "MemoryScanner::snapshot accepts untouched region",
		  .function = MemoryScannerTests::snapshot_accepts_untouched_region },
		{ .name     = "MemoryScanner::snapshot detects content change",
		  .function = MemoryScannerTests::snapshot_detects_content_change },
		{ .name     = "MemoryScanner::snapshot detects protection change",
		  .function = MemoryScannerTests::snapshot_detects_protection_change },
		{ .name     = "MemoryScanner::snapshot detects unmapped region",
		  .function = MemoryScannerTests::snapshot_detects_unmapped_region }
	};

	constexpr Test tests_memory_map[] = {
		{ .name     = "MemoryMap::refresh current process",
		  .function = MemoryMapTests::refresh_current_process },
		{ .name     = "MemoryMap::reject invalid process",
		  .function = MemoryMapTests::reject_invalid_process },
		{ .name     = "MemoryMap::regions are sorted",
		  .function = MemoryMapTests::regions_are_sorted },
		{ .name     = "MemoryMap::find known address",
		  .function = MemoryMapTests::find_known_address },
		{ .name     = "MemoryMap::reject unmapped address",
		  .function = MemoryMapTests::reject_unmapped_address },
		{ .name     = "MemoryMap::covers contiguous range",
		  .function = MemoryMapTests::covers_contiguous_range },
		{ .name     = "MemoryMap::filter by protection",
		  .function = MemoryMapTests::filter_by_protection },
		{ .name     = "MemoryMap::filter by address window",
		  .function = MemoryMapTests::filter_by_address_window },
		{ .name     = "MemoryMap::resolve own image",
		  .function = MemoryMapTests::resolve_own_image },
		{ .name     = "MemoryMap::describe protection",
		  .function = MemoryMapTests::describe_protection },
		{ .name     = "MemoryMap::parse platform region",
		  .function = MemoryMapTests::parse_platform_region }
	};

	std::size_t total  = 0;
	std::size_t passed = 0;
	std::size_t failed = 0;

	std::size_t current = 0;

	if ( runTests( tests_settings, passed, failed, total, current ) )
	{
		return EXIT_FAILURE;
	}

	if ( runTests( tests_hashing, passed, failed, total, current ) )
	{
		return EXIT_FAILURE;
	}

	if ( runTests( tests_memory_map, passed, failed, total, current ) )
	{
		return EXIT_FAILURE;
	}

	if ( runTests( tests_memory_scanner, passed, failed, total, current ) )
	{
		return EXIT_FAILURE;
	}

	std::print( "\n {}/{} test passed, {} failed\n", passed, total, failed );

	return EXIT_SUCCESS;
}
