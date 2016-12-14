#ifndef RFUTILITIES_H
#define RFUTILITIES_H

#include <fstream>
#include <vector>
#include <string>

namespace thesisUtilities
{
	enum heartPresent_t : unsigned char
	{
		hpNone = 0,
		hpPresent,
		hpObscured
	};

	// Struct representing all the annotation information for one substructure
	struct subStructLabel_t
	{
		int x;
		int y;
		int ori;
		int present;
		bool labelled;
		subStructLabel_t() : x(-1), y(-1), ori(0), present(0), labelled(false) {}
	};

	float getFrameRate(std::string filename,std::string viddir);


	bool readTrackFile(const std::string& filename, const int n_frames, bool& headup, int& radius, std::vector<bool>& labelled_track, std::vector<heartPresent_t>& heart_present_track,
		               std::vector<int>& centrey_track, std::vector<int>& centrex_track, std::vector<int>& ori_track, std::vector<int>& view_label_track, std::vector<int>& phase_point_track, std::vector<float>& cardiac_phase_track);

	bool readSubstructuresTrackFile(const std::string& filename, const int n_frames, std::vector<std::string>& structure_names, std::vector<std::vector<subStructLabel_t>>& track);

}

// inclusion guard
#endif
