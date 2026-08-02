.pragma library

// Mirrors Date::isLeapYear() in core/revolif_backend.cpp
function isLeapYear(year) {
    return (year % 4 === 0 && year % 100 !== 0) || (year % 400 === 0);
}

// Mirrors Date::daysInMonth() in core/revolif_backend.cpp. month is 1-12.
function daysInMonth(month, year) {
    var days = [31, isLeapYear(year) ? 29 : 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];
    return days[month - 1];
}

// Mirrors Date::isPastDate() — compares whole calendar dates, not time-of-day.
// day/month(1-12)/year are the values being checked.
function isPastDate(day, month, year) {
    var today = new Date();
    today.setHours(0, 0, 0, 0);
    var candidate = new Date(year, month - 1, day);
    candidate.setHours(0, 0, 0, 0);
    return candidate.getTime() < today.getTime();
}

// Returns an error string describing the problem with day/month(1-12)/year,
// or an empty string if the date is a valid calendar date.
function validateCalendarDate(day, month, year) {
    var maxDay = daysInMonth(month, year);
    if (day < 1 || day > maxDay) {
        return "That month only has " + maxDay + " days in " + year + ".";
    }
    return "";
}
