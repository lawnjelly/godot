#include "lbitimage.h"
#include "core/image.h"
#include "core/os/file_access.h"
#include "core/project_settings.h"

namespace LM { // namespace start

#define PF_FOURCC(s) ((uint32_t)(((s)[3] << 24U) | ((s)[2] << 16U) | ((s)[1] << 8U) | ((s)[0])))

void LBitImage::create(uint32_t p_width, uint32_t p_height, bool p_blank) {
	_width = p_width;
	_height = p_height;
	_bf.create(_width * _height, p_blank);
}
void LBitImage::destroy() {
	_bf.destroy();
	_width = 0;
	_height = 0;
}

void LBitImage::copy_from(const LBitImage &p_source) {
	_bf.copy_from(p_source._bf);
	_width = p_source.get_width();
	_height = p_source.get_height();
}

bool LBitImage::shrink_x2() {
	if (!get_num_pixels())
		return false;
	if ((_width % 2) != 0)
		return false;
	if ((_height % 2) != 0)
		return false;

	LBitImage temp;
	temp.copy_from(*this);

	create(_width / 2, _height / 2);

	for (uint32_t y = 0; y < _height; y++) {
		uint32_t yy = y * 2;

		for (uint32_t x = 0; x < _width; x++) {
			uint32_t xx = x * 2;

			if (temp.get_pixel(xx, yy, false)) {
				set_pixel(x, y);
			}
		}
	}

	return true;
}

uint32_t LBitImage::count(bool p_count_set) const {
	uint32_t count = 0;
	for (uint32_t n = 0; n < get_num_pixels(); n++) {
		if (_bf.get_bit(n))
			count++;
	}

	return p_count_set ? count : get_num_pixels() - count;
}

Error LBitImage::load(String p_filename, uint8_t *r_extra_data, uint32_t *r_extra_data_size, uint32_t p_max_extra_data_size) {
	print_line("Loading : " + p_filename);
	//return ERR_SKIP;

	String path = ProjectSettings::get_singleton()->globalize_path(p_filename);

	Error err;
	FileAccess *f = FileAccess::open(path, FileAccess::READ, &err);
	if (!f) {
		return err;
	}
	destroy();

	uint32_t fourcc;
	uint32_t version;
	uint32_t width;
	uint32_t height;
	uint32_t extra_data_size;
	uint32_t num_bytes;

	if (err != OK) {
		goto cleanup;
	}

	fourcc = f->get_32();
	if (fourcc != PF_FOURCC("BitI")) {
		err = ERR_FILE_CORRUPT;
		goto cleanup;
	}

	version = f->get_32();
	if (version != 100) {
		err = ERR_FILE_UNRECOGNIZED;
		goto cleanup;
	}

	width = f->get_32();
	height = f->get_32();

	// Just some reasonable max size.
	if ((width * height) > (8192 * 8192)) {
		err = ERR_FILE_CORRUPT;
		goto cleanup;
	}
	create(width, height);

	num_bytes = f->get_32();
	if (num_bytes != _bf.get_num_bytes()) {
		err = ERR_FILE_CORRUPT;
		goto cleanup;
	}

	extra_data_size = f->get_32();
	if (extra_data_size) {
		if (extra_data_size > p_max_extra_data_size) {
			err = ERR_FILE_CORRUPT;
			goto cleanup;
		}
		if (!r_extra_data || !r_extra_data_size) {
			err = ERR_INVALID_PARAMETER;
			goto cleanup;
		}

		if (f->get_buffer(r_extra_data, extra_data_size) != extra_data_size) {
			err = ERR_FILE_CORRUPT;
			goto cleanup;
		}
	}

	if (f->get_buffer(_bf.get_data(), num_bytes) != num_bytes) {
		err = ERR_FILE_CORRUPT;
		goto cleanup;
	}

cleanup:
	memdelete(f);
	return err;
}

Error LBitImage::save_png(String p_filename) {
	if (p_filename.get_extension() != "png") {
		return ERR_FILE_BAD_PATH;
	}

	Ref<Image> image = memnew(Image(get_width(), get_height(), false, Image::FORMAT_L8));

	image->lock();

	for (uint32_t y = 0; y < _height; y++) {
		for (uint32_t x = 0; x < _width; x++) {
			bool set = get_pixel(x, y);
			image->set_pixel(x, y, set ? Color(1, 1, 1, 1) : Color(0, 0, 0, 0));
		}
	}
	image->unlock();

	String global_path = ProjectSettings::get_singleton()->globalize_path(p_filename);
	print_line("saving BitImage .. global path : " + global_path);

	Error err = image->save_png(global_path);

	if (err != OK)
		OS::get_singleton()->alert("Error writing PNG file. Does this folder exist?\n\n" + global_path, "WARNING");

	return err;
}

Error LBitImage::save(String p_filename, const uint8_t *p_extra_data, uint32_t p_extra_data_size) {
	print_line("Saving : " + p_filename);

	String path = ProjectSettings::get_singleton()->globalize_path(p_filename);

	Error err;
	FileAccess *f = FileAccess::open(path, FileAccess::WRITE, &err);
	if (!f) {
		return err;
	}

	if (err != OK) {
		goto cleanup;
	}

	f->store_string("BitI");
	f->store_32(100); // version
	f->store_32(get_width());
	f->store_32(get_height());
	f->store_32(_bf.get_num_bytes());
	f->store_32(p_extra_data_size);

	if (p_extra_data_size) {
		if (!p_extra_data) {
			err = ERR_INVALID_PARAMETER;
			goto cleanup;
		}

		f->store_buffer(p_extra_data, p_extra_data_size);
	}

	f->store_buffer(_bf.get_data(), _bf.get_num_bytes());
	f->flush();

cleanup:
	memdelete(f);
	return err;
}

} //namespace LM
