// ED9FontConverter.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <OpenXLSX.hpp>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <map>
#include <ft2build.h>
#include<stdint.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <unicode/unistr.h>
#include <unicode/ucnv_cb.h>
#include "unicode/utypes.h"
#include "unicode/ucnv.h"
#include "unicode/ucnv_err.h"
#include <unicode/chariter.h>

#define CHAR_HEIGHT 48
#define CHAR_WIDTH 48
#include FT_FREETYPE_H

using namespace OpenXLSX;

struct character {
    int code;
    int XSpacing;
    int YOffset;
    int NoIdea;
	int OriginY;
	int OriginX;
	int Width;
	int Height;
	bool green;
	int YBearing;
	bool non_existent = false;
	character() = default;
    character(int c, int x, int y, int idk): code(c), XSpacing(x), YOffset(y), NoIdea(idk){
    }
};

std::vector<unsigned char> GetBytesFromInt(int i) {
	std::vector<unsigned char> result;
	result.push_back(i & 0x000000ff);
	result.push_back((i & 0x0000ff00) >> 8);
	result.push_back((i & 0x00ff0000) >> 16);
	result.push_back((i) >> 24);
	return result;
}

std::vector<unsigned char> GetBytesFromShort(uint16_t i) {
	std::vector<unsigned char> result;
	result.push_back(i & 0xff);
	result.push_back((i & 0xff00) >> 8);
	return result;
}
void AddShortToVector(std::vector<uint8_t>& vec, uint16_t sh) {
	std::vector<uint8_t> sh_b = GetBytesFromShort(sh);
	vec.insert(vec.end(), sh_b.begin(), sh_b.end());
}
void AddIntToVector(std::vector<uint8_t>& vec, uint32_t i) {
	std::vector<uint8_t> i_b = GetBytesFromInt(i);
	vec.insert(vec.end(), i_b.begin(), i_b.end());
}

bool generate_new_XSpacing_and_YOffset_xlsx(std::string font_name, std::vector<std::string> fallback_fonts, std::map<uint32_t, character> chars) {

	FT_Library    library;
	FT_GlyphSlot  slot;
	FT_Matrix     matrix = FT_Matrix();
	FT_Vector     pen = FT_Vector();
	FT_Error      error;
	FT_Int		  char_width = CHAR_WIDTH;
	FT_Int	      char_height = CHAR_HEIGHT;


	double        angle = 0 * 3.14 / 2;
	int           target_height;

	int code;
	int idx;

	int nb_char = chars.size();

	int current_green_pointer = 0;

	int origin_y = ceil(char_height * 0.2);

	int  char_code;
	FT_Face       face;

	bool green = true;
	error = FT_Init_FreeType(&library);

	auto char_it = chars.begin();

	int maxBearing = -1;

	for (idx = 0; idx < nb_char; idx += 1) {


		error = FT_New_Face(library, font_name.c_str(), 0, &face);
		char_code = char_it->second.code;
		
		code = FT_Get_Char_Index(face, char_code);
		if (code == 0) {
			
			//printf("code : %x n'existe pas\n", code);
			//std::cout << "Character doesnt exist " << std::hex << char_code << std::endl;
			for (std::string fallback : fallback_fonts) {
				FT_Done_Face(face);
				error = FT_New_Face(library, fallback.c_str(), 0, &face);
				code = FT_Get_Char_Index(face, char_code);
				if (code != 0)
					break;
			}
			if (code == 0)
			{
				char_it->second.XSpacing = -1;
				char_it->second.YOffset = -1;
				char_it->second.YBearing = -1;
			}
		}
		if (code != 0) {
			error = FT_Set_Pixel_Sizes(face,
				char_width,
				char_height);

			slot = face->glyph;
			FT_Load_Glyph(face, code, FT_LOAD_RENDER);

			char_it->second.YBearing = face->glyph->metrics.horiBearingY / 64;
			if (maxBearing < char_it->second.YBearing)
				maxBearing = char_it->second.YBearing;

			char_it->second.XSpacing = slot->bitmap.width + 2;
			
			FT_Done_Face(face);

		}
		char_it++;
	}
	FT_Done_FreeType(library);
	
	for (auto char_it = chars.begin(); char_it != chars.end(); char_it++) {
		char_it->second.YOffset = maxBearing - char_it->second.YBearing;
	}
	std::cout << "Max: "<< maxBearing << std::endl;
	XLDocument doc;
	doc.create("./font_spacings.xlsx");
	auto wks = doc.workbook().worksheet("Sheet1");
	size_t cnter = 2;
	for (auto char_it = chars.begin(); char_it != chars.end(); char_it++) {
		auto cell_char = wks.cell(cnter, 1);
		auto cell_Y = wks.cell(cnter, 2);
		auto cell_X = wks.cell(cnter, 3);
		auto cell_YBearing = wks.cell(cnter, 4);
		UChar32 char32 = char_it->second.code;
		icu::UnicodeString src(char32);

		std::string converted;
		src.toUTF8String(converted);

		size_t length = src.extract(0, 1, NULL, 0, "utf8");
		char* buffer = new char[length];
		src.extract(0, 1, buffer, length, "utf8");
		/*for (auto i = 0; i < length; i++)
			std::cout << std::hex << (int) (buffer[i] & 0x000000FF) << " ";
		std::cout << std::endl;*/
		cell_char.value() = converted;
		cell_Y.value() = char_it->second.YOffset;
		cell_X.value() = char_it->second.XSpacing;
		cell_YBearing.value() = char_it->second.YBearing;
		cnter++;

		delete[] buffer;
	}
	doc.save();


}

bool write_string_in_png(std::string s, std::string font_name, std::vector<std::string> fallback_fonts, std::map<uint32_t, character> chars) {



	FT_Library    library;
	FT_GlyphSlot  slot;
	FT_Matrix     matrix = FT_Matrix();
	FT_Vector     pen = FT_Vector();
	FT_Error      error;
	FT_Int		  char_width = CHAR_WIDTH;
	FT_Int	      char_height = CHAR_HEIGHT;
	FT_Face       face;
	error = FT_Init_FreeType(&library);
	double        angle = 0 * 3.14 / 2;
	icu::UnicodeString src(s.c_str());
	int origin_y = ceil(char_height * 0.2);
	unsigned int code;

	size_t image_width = char_width * src.length() + 500;
	size_t image_height = char_height * 2;
	size_t full_image_size = image_width * image_height * 3; //RGB
	char* output_file = new char[full_image_size];
	memset(output_file, 0, full_image_size);
	unsigned current_green_pointer = 0;
	for (int32_t i = 0; i < src.length(); i = src.moveIndex32(i, 1))
	{
		unsigned int char_code = src.char32At(i);

		error = FT_New_Face(library, font_name.c_str(), 0, &face);
		code = FT_Get_Char_Index(face, char_code);
		if (code == 0) {
			//printf("code : %x n'existe pas\n", code);
			//std::cout << "Character doesnt exist " << std::hex << char_code << std::endl;
			
			for (std::string fallback : fallback_fonts) {
				FT_Done_Face(face);
				error = FT_New_Face(library, fallback.c_str(), 0, &face);
				code = FT_Get_Char_Index(face, char_code);
				if (code != 0)
					break;
			}

			/*if (code == 0)
				std::cout << "Unicode doesnt exist in the fall back font  " << std::hex << char_code << std::endl;*/
		}
		if (code != 0) {
			//std::cout << "Character exists " << std::hex << char_code << std::endl;
			error = FT_Set_Pixel_Sizes(face,
				char_width,
				char_height);

			slot = face->glyph;

			//printf("code : %x existe\n", code);
			FT_Load_Glyph(face, code, FT_LOAD_RENDER);

			int activate = 0;
			uint8_t cur_pix_to_render = 0x0;
			int left_offset = 0;


			int idx_byte_output = 0;
			activate = 0;
			int tot = 0;

			FT_Int yBearing = face->glyph->metrics.horiBearingY / 64;

			FT_Int yoff = 0;//c.Yoffset;

			FT_Int image_top_row = origin_y + yBearing + yoff;
			auto bitmap = &slot->bitmap;
			unsigned int imageIndex;
			unsigned int origin = 0;
			unsigned int bitmapIndex;
			int current_row;
			origin = current_green_pointer;

			for (int y = 0; y < bitmap->rows; y++) {

				for (unsigned int x = 0; x < bitmap->width; x++) {
					imageIndex = origin + x * 3 + (chars[char_code].YOffset +  y) * (image_width * 3); //
					bitmapIndex = x + y * bitmap->width;

					if (bitmapIndex >= bitmap->width * bitmap->rows) break;

					unsigned int R_addr = imageIndex;
					unsigned int G_addr = (imageIndex + 1);
					if ((R_addr >= 0) && (G_addr < full_image_size)) {

						size_t index;
						index = imageIndex;
						output_file[index] = bitmap->buffer[bitmapIndex];
					}

				}
			}

			current_green_pointer = origin + chars[char_code].XSpacing * 3;
			FT_Done_Face(face);


		}
		

	}
	FT_Done_FreeType(library);
	stbi_write_png("text.png", image_width, image_height, 3, output_file, image_width * sizeof(uint8_t) * 3);

	delete[] output_file;

}

int main(int argc, char* argv[])
{
	if (argc >= 4) {
		size_t image_width = 4096;
		size_t image_height = 4096;
		size_t full_image_size = image_width * image_height * 3; //RGB
		char * output_file = new char[full_image_size];
		memset(output_file, 0, full_image_size);
		std::vector<std::string> fallback_fonts = {};
		std::string font_name = std::string(argv[1]);
		XLDocument InputFNT = XLDocument(argv[2]);
		for (unsigned int i = 3; i < argc; i++) {
			std::string fallback_font = std::string(argv[i]);
			std::cout << "adding font: " << fallback_font << std::endl;
			fallback_fonts.push_back(fallback_font);
		}
		
		auto wkbk = InputFNT.workbook();
		size_t sheet_cnt = wkbk.sheetCount();
		auto sheet_names = wkbk.sheetNames();

		std::map<uint32_t, character> characters;

		auto current_sheet = wkbk.worksheet(sheet_names[0]);


		size_t cols = current_sheet.columnCount();
		size_t rows = current_sheet.rowCount();
		std::cout << rows << std::endl;
		for (unsigned int i_row = 2; i_row < rows + 1; i_row++) {
			auto current_cell_char = current_sheet.cell(i_row, 1);
			auto current_cell_x = current_sheet.cell(i_row, 2);
			auto current_cell_y = current_sheet.cell(i_row, 3);
			auto current_cell_no_idea = current_sheet.cell(i_row, 4);
			std::string type = current_cell_char.value().typeAsString();
			std::string charac_str;
			if (type.compare("string") == 0)
				charac_str = current_cell_char.value().get<std::string>();
			else if (type.compare("integer") == 0)
				charac_str = std::to_string(current_cell_char.value().get<int>());
			icu::UnicodeString src(charac_str.c_str(), "utf8"); //read as utf8
			
			
			/*UChar32 utf32[1];
			UErrorCode errorCode = U_ZERO_ERROR;
			int32_t length = src.toUTF32(utf32, 1, errorCode); //convert it from Utf8 to Utf32

			std::cout << std::hex << "code : " << utf32[0] << std::endl;*/
			character chr = character(src.char32At(0),
				current_cell_x.value().get<int32_t>(),
				current_cell_y.value().get<int32_t>(),
				current_cell_no_idea.value().get<int32_t>()
			);
			characters[src.char32At(0)] = (chr);


		}
		//We got all the characters needed.
		FT_Library    library;
		FT_GlyphSlot  slot;
		FT_Matrix     matrix = FT_Matrix();
		FT_Vector     pen = FT_Vector();
		FT_Error      error;
		FT_Int		  char_width = CHAR_WIDTH;
		FT_Int	      char_height = CHAR_HEIGHT;


		double        angle = 0 * 3.14 / 2;
		int           target_height;

		int code;
		int idx;

		write_string_in_png("test lol ypqzZWRkK@", font_name, fallback_fonts, characters);
		generate_new_XSpacing_and_YOffset_xlsx(font_name, fallback_fonts, characters);
		int nb_char = characters.size();

		int current_green_pointer = 0;
		int current_red_pointer = 0;

		int origin_y = ceil(char_height * 0.2);

		int  char_code;
		FT_Face       face;

		bool green = true;
		error = FT_Init_FreeType(&library);

		auto char_it = characters.begin();

		for (idx = 0; idx < nb_char; idx += 1) {
			error = FT_New_Face(library, font_name.c_str(), 0, &face);
			char_code = char_it->second.code;
			code = FT_Get_Char_Index(face, char_code);
			if (code == 0) {
				
				//printf("code : %x n'existe pas\n", code);
				//std::cout << "Character doesnt exist " << std::hex << char_code << std::endl;

				for (std::string fallback : fallback_fonts) {
					FT_Done_Face(face);
					error = FT_New_Face(library, fallback.c_str(), 0, &face);
					code = FT_Get_Char_Index(face, char_code);
					if (code != 0)
						break;
				}
				
				if (code == 0) {
					std::cout << "Unicode doesnt exist in the fallback fonts  " << std::hex << char_code << std::endl;
					char_it->second.non_existent = true;
				}
					
			}
			if (code != 0) {
				//std::cout << "Character exists " << std::hex << char_code << std::endl;
				error = FT_Set_Pixel_Sizes(face,
					char_width,
					char_height);

				slot = face->glyph;

				

				//printf("code : %x existe\n", code);
				FT_Load_Glyph(face, code, FT_LOAD_RENDER);


				int activate = 0;
				uint8_t cur_pix_to_render = 0x0;
				int left_offset = 0;


				int idx_byte_output = 0;
				activate = 0;
				int tot = 0;

				FT_Int yBearing = face->glyph->metrics.horiBearingY / 64;

				FT_Int yoff = 0;//c.Yoffset;

				FT_Int image_top_row = origin_y + yBearing + yoff;
				auto bitmap = &slot->bitmap;
				unsigned int imageIndex;
				unsigned int origin = 0;
				unsigned int bitmapIndex;
				int current_row;

				if (green) {
					origin = current_green_pointer;
					
				}
				else {
					origin = current_red_pointer;

				}
				current_row = origin / (image_width * 3);
				unsigned int current_col = (origin - current_row * (image_width * 3))/3;

				if ((current_col + bitmap->width) > image_width) {
					
					size_t letter_row_unit = (char_height * image_width * 3); // 48 * 4096 * 3 bytes
					size_t nb_letter_rows = origin / letter_row_unit;
					origin = (nb_letter_rows + 1) * letter_row_unit;
					current_row = origin / (image_width * 3);
					current_col = (origin - current_row * (image_width * 3)) / 3;
				}

				char_it->second.green = green;
				char_it->second.Width = bitmap->width;
				char_it->second.Height = bitmap->rows;
				char_it->second.OriginX = current_col;
				char_it->second.OriginY = current_row;


				for (int y = bitmap->rows - 1; y >= 0; y--) {

					for (unsigned int x = 0; x < bitmap->width; x++) {
						imageIndex = origin + x * 3 + (y) * (image_width * 3);
						bitmapIndex = x + y * bitmap->width;

						if (bitmapIndex >= bitmap->width * bitmap->rows) break;

						unsigned int R_addr = imageIndex;
						unsigned int G_addr = (imageIndex + 1);
						if ((R_addr >= 0) && (G_addr < full_image_size)) {

							size_t index;

							if (green)
								index = imageIndex + 1;
							else
								index = imageIndex;
							output_file[index] = bitmap->buffer[bitmapIndex];
						}

					}
				}

				if (green) {

					current_green_pointer = origin + (bitmap->width + 2)* 3;

				}
				else {
					current_red_pointer = origin + (bitmap->width + 2) * 3 ;
				}


				FT_Done_Face(face);
				

				green = !green;
			}
			char_it++;
		}

		for (auto it = characters.cbegin(); it != characters.cend(); )
		{

			if (it->second.non_existent)
			{
				characters.erase(it++);   
			}
			else
			{
				++it;
			}
		}

		FT_Done_FreeType(library);
		stbi_write_tga("font0.tga", image_width, image_height, 3, output_file);
		delete[] output_file;

		//generating the fnt
		std::vector<uint8_t> fnt_file_head = { 0x46, 0x43, 0x56, 0x00, 0x02, 0x00, 0x20, 0x00 };
		AddIntToVector(fnt_file_head, characters.size());
		AddShortToVector(fnt_file_head, 0x44);
		AddShortToVector(fnt_file_head, 0x03);
		AddShortToVector(fnt_file_head, 0x01);
		AddShortToVector(fnt_file_head, 0x01);
		AddIntToVector(fnt_file_head, 0);
		AddIntToVector(fnt_file_head, 0);
		AddIntToVector(fnt_file_head, 0);
		AddIntToVector(fnt_file_head, 0x49544C46);
		std::vector<uint8_t> characters_bin = {};

		for (auto it = characters.cbegin(); it != characters.cend(); it++)
		{
			AddIntToVector(characters_bin, it->second.code);
			AddIntToVector(characters_bin, 1);
			AddShortToVector(characters_bin, it->second.OriginX);
			AddShortToVector(characters_bin, it->second.OriginY);
			AddShortToVector(characters_bin, it->second.Width);
			AddShortToVector(characters_bin, it->second.Height);
			if (it->second.green)
				AddShortToVector(characters_bin, 0x100);
			else 
				AddShortToVector(characters_bin, 0x200);
			AddShortToVector(characters_bin, 0);
			AddShortToVector(characters_bin, it->second.YOffset);
			AddShortToVector(characters_bin, it->second.XSpacing);
		}
		AddIntToVector(fnt_file_head, characters_bin.size());
		std::vector<uint8_t> fnt_file = fnt_file_head;
		fnt_file.insert(fnt_file.end(), characters_bin.begin(), characters_bin.end());
	
		std::ofstream outfile2("font_0.fnt", std::ios::out | std::ios::binary);
		outfile2.write((const char*)fnt_file.data(), fnt_file.size());
		outfile2.close();

	}
}

