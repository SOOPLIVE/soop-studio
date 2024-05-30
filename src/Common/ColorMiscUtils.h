#pragma once


static inline long long color_to_int(const QColor& color)
{
	auto shift = [&](unsigned val, int shift) {
		return ((val & 0xff) << shift);
		};

	return shift(color.red(), 0) | shift(color.green(), 8) |
		shift(color.blue(), 16) | shift(color.alpha(), 24);
}

static inline QColor rgba_to_color(uint32_t rgba)
{
	return QColor::fromRgb(rgba & 0xFF, (rgba >> 8) & 0xFF,
		(rgba >> 16) & 0xFF, (rgba >> 24) & 0xFF);
}
