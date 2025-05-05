#pragma once
#include <chrono>
#include <string>

using namespace std::chrono_literals;
using namespace std::string_literals;

unsigned getDayFromDate(const std::chrono::time_point<std::chrono::system_clock>& time) {
	std::chrono::year_month_day ymd(std::chrono::floor<std::chrono::days>(time));
	return unsigned(ymd.day());
}
unsigned getMonthFromDate(const std::chrono::time_point<std::chrono::system_clock>& time) {
	std::chrono::year_month_day ymd(std::chrono::floor<std::chrono::days>(time));
	return unsigned(ymd.month());
}

unsigned getLastDayOfMonth(const std::chrono::time_point<std::chrono::system_clock>& time) {
	std::chrono::year_month_day ymd(std::chrono::floor<std::chrono::days>(time));
	std::chrono::month_day_last mdl(ymd.month() / std::chrono::last);
	std::chrono::year_month_day_last ymdl(ymd.year(), mdl);
	return unsigned(ymdl.day());
}

// Example of output: 2024-09-25T09:01:05.000Z
std::string formDateString(const std::chrono::time_point<std::chrono::system_clock>& time) {
	std::string output;
	output.reserve(24);
	std::chrono::year_month_day ymd(std::chrono::floor<std::chrono::days>(time));
	std::chrono::hh_mm_ss hms(time - std::chrono::floor<std::chrono::days>(time));
	output += std::to_string(int(ymd.year())) + '-';
	if (unsigned(ymd.month()) < 10) {
		output += '0';
	}
	output += std::to_string(unsigned(ymd.month())) + '-';
	if (unsigned(ymd.day()) < 10) {
		output += '0';
	}
	output += std::to_string(unsigned(ymd.day())) + 'T';
	if (hms.hours().count() < 10) {
		output += '0';
	}
	output += std::to_string(hms.hours().count()) + ':';
	if (hms.minutes().count() < 10) {
		output += '0';
	}
	output += std::to_string(hms.minutes().count()) + ':';
	if (hms.seconds().count() < 10) {
		output += '0';
	}
	output += std::to_string(hms.seconds().count()) + '.';
	output += "000Z"s;
	return output;
}

// Same but std::wstring
std::wstring formDateWString(const std::chrono::time_point<std::chrono::system_clock>& time) {
	std::wstring output;
	output.reserve(24);
	std::chrono::year_month_day ymd(std::chrono::floor<std::chrono::days>(time));
	std::chrono::hh_mm_ss hms(time - std::chrono::floor<std::chrono::days>(time));
	output += std::to_wstring(int(ymd.year())) + L'-';
	if (unsigned(ymd.month()) < 10) {
		output += L'0';
	}
	output += std::to_wstring(unsigned(ymd.month())) + L'-';
	if (unsigned(ymd.day()) < 10) {
		output += L'0';
	}
	output += std::to_wstring(unsigned(ymd.day())) + L'T';
	if (hms.hours().count() < 10) {
		output += L'0';
	}
	output += std::to_wstring(hms.hours().count()) + L':';
	if (hms.minutes().count() < 10) {
		output += L'0';
	}
	output += std::to_wstring(hms.minutes().count()) + L':';
	if (hms.seconds().count() < 10) {
		output += L'0';
	}
	output += std::to_wstring(hms.seconds().count()) + L'.';
	output += L"000Z";
	return output;
}

std::string getRelativeDate(std::chrono::time_point<std::chrono::system_clock> time, int year_change ,int day_change) {
	time += std::chrono::days(day_change);
	time += std::chrono::years(year_change);
	return formDateString(time);
}

std::wstring getRelativeDateW(std::chrono::time_point<std::chrono::system_clock> time, int year_change, int day_change) {
	time += std::chrono::days(day_change);
	time += std::chrono::years(year_change);
	return formDateWString(time);
}