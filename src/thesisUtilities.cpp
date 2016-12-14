#include "thesisUtilities.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cmath>

#define FRAME_RATE_DATABASE "frameratedatabase"

using namespace std;

namespace thesisUtilities
{


// Utility function to read in the frame rate from a database file
// Used because sometimes opencv cannot find the correct frame rate
float getFrameRate(string filename, string viddir)
{
	string vidname;
	float frame_rate = nan(""), temp;

	filename = filename.substr(filename.find_last_of("/")+1,string::npos);

	// Append trailing slash if necessary
	if(viddir.at(viddir.length() -1 ) != '/')
		viddir += '/';

	ifstream infile(viddir + FRAME_RATE_DATABASE);
	if(!infile.is_open())
	{
		cout << "Could not open frame rate database file " << viddir + FRAME_RATE_DATABASE << endl;
		return frame_rate;
	}

	while (infile >> vidname)
	{
		infile >> temp;

		if(filename.compare(vidname) == 0)
		{
			frame_rate = temp;
			break;
		}
	}

	infile.close();

	return frame_rate;

}


bool readTrackFile(const string& filename, const int n_frames, bool& headup, int& radius, vector<bool>& labelled_track, vector<heartPresent_t>& heart_present_track,
	               vector<int>&  centrey_track, vector<int>&  centrex_track, vector<int>&  ori_track, vector<int>&  view_label_track, vector<int>&  phase_point_track, vector<float>& cardiac_phase_track)
{
	// Resize the arrays
	labelled_track.resize(n_frames);
	heart_present_track.resize(n_frames);
	centrex_track.resize(n_frames);
	centrey_track.resize(n_frames);
	ori_track.resize(n_frames);
	view_label_track.resize(n_frames);
	phase_point_track.resize(n_frames);
	cardiac_phase_track.resize(n_frames);

	ifstream infile(filename.c_str());
	if (infile.is_open())
	{
		string dummy_string;

		// Skip the first and second lines - a header line
		// and video dimensions respectively
		getline(infile,dummy_string);
		getline(infile,dummy_string);

		infile >> headup;
		if(infile.fail())
			return false;

		infile >> radius;
		if(infile.fail())
			return false;

		for(int f = 0; f < n_frames; f++)
		{
			int dummy_int;
			infile >> dummy_int;
			if(infile.fail() || dummy_int != f)
			{
				if(infile.eof()) // we've reached the end of the file before we expected to, mark the other frames as unlabelled
				{
					for(int l = f; l < n_frames; ++l)
					{
						labelled_track[l] = false;
						heart_present_track[f] = hpNone;
						centrey_track[l] = 0;
						centrex_track[l] = 0;
						ori_track[l] = 0;
						view_label_track[l] = 0;
						phase_point_track[l] = 0;
						cardiac_phase_track[l] = 0;
					}
					break;
				}
				else
					return false;
			}

			bool tempbool;
			infile >> tempbool;
			if(infile.fail())
				return false;
			labelled_track[f] = tempbool;

			infile >> dummy_int;
			if(infile.fail())
				return false;
			heart_present_track[f] = heartPresent_t(dummy_int);

			infile >> centrey_track[f];
			if(infile.fail())
				return false;

			infile >> centrex_track[f];
			if(infile.fail())
				return false;

			infile >> ori_track[f];
			if(infile.fail())
				return false;

			infile >> view_label_track[f];
			if(infile.fail())
				return false;

			infile >> phase_point_track[f];
			if(infile.fail())
				return false;

			infile >> cardiac_phase_track[f];
			if(infile.fail())
				return false;

		}

		infile.close();

		return true;
	}
	else
		return false;
}


bool readSubstructuresTrackFile(const std::string& filename, const int n_frames, std::vector<std::string>& structure_names, std::vector<std::vector<subStructLabel_t>>& track)
{
	ifstream infile(filename.c_str());
	if (infile.is_open())
	{
		string dummy_string;
		// Skip the first, header line
		getline(infile,dummy_string);

		int n_structures;
		infile >> n_structures;
		if(infile.fail())
			return false;

		// Skip the rest of this line
		getline(infile,dummy_string);

		structure_names.resize(n_structures);

		track.resize(n_frames);
		for(vector<subStructLabel_t>& v : track)
			v.resize(n_structures);

		// Loop through the substructures
		for(int s = 0; s < n_structures; ++s)
		{
			// Read in the first line with name and number
			int dummy_int;
			infile >> dummy_int;
			if(infile.fail())
				return false;

			// Check the structure number matches what we expectedSize
			if(dummy_int != s)
				return false;

			// Read in the name
			infile >> structure_names[s];
			if(infile.fail())
				return false;

			// Loop through frames
			getline(infile,dummy_string);
			int f = -1;
			for(string linestring; !getline(infile,linestring).eof() && !linestring.empty(); )
			{
				f++;
				stringstream ss(linestring);
				ss >> dummy_int;
				if(ss.fail())
					return false;

				if(dummy_int != f)
					return false;

				ss >> track[f][s].labelled;
				if(ss.fail())
					return false;
				ss >> track[f][s].present;
				if(ss.fail())
					return false;
				ss >> track[f][s].y;
				if(ss.fail())
					return false;
				ss >> track[f][s].x;
				if(ss.fail())
					return false;
				ss >> track[f][s].ori;
				if(ss.fail())
					return false;
			}
			// We do not have information on some of the frames at the end
			// Mark them as unlabelled
			while(f < n_frames - 1)
			{
				track[f][s].labelled = false;
				track[f][s].present = 0;
				track[f][s].y = -1;
				track[f][s].x = -1;
				track[f][s].ori = 0;
				f++;
			}
		}
		infile.close();
		return true;
	}
	else
		return false;
}

} // end of namespace
