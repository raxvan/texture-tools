#include <vector>
#include <iostream>
#include <stdlib.h>
#include <algorithm>
#include <array>
#include <charconv>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


struct data
{
	int w;
	int h;

	uint8_t* pixels;

	std::vector<float> fpixels;
	std::vector<float> dst;
};

int main(int argc, const char ** argv)
{
	std::string input_file_path;
	std::string output_file_path;

	std::array<float,3>* alpha_blending = nullptr;//blending with some color
	bool premultiply_alpha = false;


	data image;
	std::array<float, 3> blending_color = {0.0f,0.0f,0.0f};//if any;

	{
		bool valid = true;
		//args
		std::vector < std::string > args;
		for (int i = 0; i < argc; i++)
			args.push_back(argv[i]);

		auto getarg = [&](const std::string& a) -> std::string
		{
			auto itr = std::find_if(args.begin(), args.end(), [&](const std::string& v) {
				return a == v;
				});

			if (itr != args.end())
			{
				itr = args.erase(itr);
				if (itr == args.end())
				{
					std::cerr << "Incomplete argument " << a << std::endl;
					valid = false;
					return std::string();
				}

				auto result = *itr;
				args.erase(itr);
				return result;
			}
			return std::string();
		};

		input_file_path = getarg("-i");
		output_file_path = getarg("-o");

		auto br = getarg("-br");
		auto bg = getarg("-bg");
		auto bb = getarg("-bb");

		premultiply_alpha = std::find(args.begin(),args.end(), "-p") != args.end();

		auto set_and_validate = [&](const std::string& s, float& value){
			auto [ptr, ec] = std::from_chars(s.data(),s.data() + s.size(), value);
			if (
				(ec == std::errc::invalid_argument) ||
				(ec == std::errc::result_out_of_range)
			)
			{
				std::cerr << "Error parsing " << s << std::endl;
				valid = false;
			}
		};

		if (br.size() != 0 || bg.size() != 0 || bb.size() != 0)
		{
			alpha_blending = &blending_color;
			if(br.size() > 0)
				set_and_validate(br, blending_color[0]);
			if(bg.size() > 0)
				set_and_validate(bg, blending_color[1]);
			if(bb.size() > 0)
				set_and_validate(bb, blending_color[2]);
		}

		if(valid == false)
		{
			return -1;
		}
	}

	if (input_file_path.size() == 0)
	{
		std::cerr << "Missing input -i PATH" << std::endl;
		return -1;
	}
	if (output_file_path.size() == 0)
	{
		std::cerr << "Missing output -o PATH" << std::endl;
		return -1;
	}


	{
		int comp;
		image.pixels = stbi_load(input_file_path.c_str(), &image.w, &image.h, &comp, 4);
		if(image.pixels == nullptr)
		{
			std::cerr << "Could not load image " << input_file_path << std::endl;
			return -1;
		}
		if(image.w < 2 || image.h < 2 || comp != 4)
		{
			std::cerr << "Invalid image format." << std::endl;
			return -1;
		}
	}

	image.fpixels.resize(image.w * image.h * 4);
	image.dst.resize(image.w * image.h * 4);

	//first processing step;
	for(int y = 0; y < image.h; y++)
	{
		for(int x = 0; x < image.w;x++)
		{
			int index = y * image.w + x;
			image.fpixels[index * 4 + 0] = float(image.pixels[index * 4 + 0]) / 255.0f;
			image.fpixels[index * 4 + 1] = float(image.pixels[index * 4 + 1]) / 255.0f;
			image.fpixels[index * 4 + 2] = float(image.pixels[index * 4 + 2]) / 255.0f;
			image.fpixels[index * 4 + 3] = float(image.pixels[index * 4 + 3]) / 255.0f;
		}
	}

	auto delta_alpha = [](const float* pc, const float* pnear){
		float f1 = pc[3];
		float f2 = pnear[3];
		return std::max(f2 - f1, 0.0f);
	};

	auto avg_impact = [&](const float* pc, const float* pnear){
		float da = delta_alpha(pc, pnear);
		return da;
	};

	auto mix = [](float a, float b, float f){
		return a * (1.0f - f) + b * f;
	};

	//process
	for(int y = 0; y < image.h; y++)
	{
		for(int x = 0; x < image.w;x++)
		{
			int index = y * image.w + x;
			std::array<float, 4> result = { 0.0f, 0.0f, 0.0f, 0.0f };
			auto accumulate = [&](const int nabour_index) {
				float ai = avg_impact(image.fpixels.data() + index * 4, image.fpixels.data() + nabour_index * 4);
				result[0] += image.fpixels[nabour_index * 4 + 0] * ai;
				result[1] += image.fpixels[nabour_index * 4 + 1] * ai;
				result[2] += image.fpixels[nabour_index * 4 + 2] * ai;
				result[3] += ai;
			};
			
			if(x > 0)
			{
				int i = y * image.w + x - 1;
				accumulate(i);
			}
			if(y > 0)
			{
				int i = (y - 1) * image.w + x;
				accumulate(i);
			}
			if(x < (image.w - 1))
			{
				int i = y * image.w + x + 1;
				accumulate(i);
			}
			if(y < (image.h - 1))
			{
				int i = (y + 1) * image.w + x;
				accumulate(i);
			}

			float weight_sum = result[3];

			if(weight_sum < (1.0f / 255.0f))
			{
				result[0] = image.fpixels[index * 4 + 0];
				result[1] = image.fpixels[index * 4 + 1];
				result[2] = image.fpixels[index * 4 + 2];
				weight_sum = 1.0f;
			}

			float original_alpha = image.fpixels[index * 4 + 3];
			float color_weight = original_alpha;

			image.dst[index * 4 + 0] = mix(result[0] / weight_sum, image.fpixels[index * 4 + 0], color_weight);
			image.dst[index * 4 + 1] = mix(result[1] / weight_sum, image.fpixels[index * 4 + 1], color_weight);
			image.dst[index * 4 + 2] = mix(result[2] / weight_sum, image.fpixels[index * 4 + 2], color_weight);

			if (premultiply_alpha)
			{
				image.dst[index * 4 + 0] = image.dst[index * 4 + 0] * original_alpha;
				image.dst[index * 4 + 1] = image.dst[index * 4 + 1] * original_alpha;
				image.dst[index * 4 + 2] = image.dst[index * 4 + 2] * original_alpha;

				if (alpha_blending != nullptr)
				{
					image.dst[index * 4 + 0] += (*alpha_blending)[0] * (1.0f - original_alpha);
					image.dst[index * 4 + 1] += (*alpha_blending)[1] * (1.0f - original_alpha);
					image.dst[index * 4 + 2] += (*alpha_blending)[2] * (1.0f - original_alpha);
				}
				
			}
			else if (alpha_blending != nullptr)
			{
				image.dst[index * 4 + 0] = mix((*alpha_blending)[0], image.dst[index * 4 + 0], original_alpha);
				image.dst[index * 4 + 1] = mix((*alpha_blending)[1], image.dst[index * 4 + 1], original_alpha);
				image.dst[index * 4 + 2] = mix((*alpha_blending)[2], image.dst[index * 4 + 2], original_alpha);
			}

		}
	}

	auto float_to_color = [](const float f){
		float r = f * 255.0f;
		int ri = int(r);
		if (ri > 255)
			ri = 255;
		if (ri < 0)
			ri = 0;
		return uint8_t(ri);
	};

	{
		//final update:
		for (int y = 0; y < image.h; y++)
		{
			for (int x = 0; x < image.w; x++)
			{
				int index = y * image.w + x;

				image.pixels[index * 4 + 0] = float_to_color(image.dst[index * 4 + 0]);
				image.pixels[index * 4 + 1] = float_to_color(image.dst[index * 4 + 1]);
				image.pixels[index * 4 + 2] = float_to_color(image.dst[index * 4 + 2]);
				image.pixels[index * 4 + 3] = float_to_color(image.fpixels[index * 4 + 3]);
			}
		}
	}

	int r =  stbi_write_tga(output_file_path.c_str(), image.w, image.h, 4, image.pixels);
	if (r != 1)
	{
		std::cerr << "Failed to write image " << output_file_path << std::endl;
		return -1;
	}


	return 0;
}
