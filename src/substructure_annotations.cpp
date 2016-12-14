#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <boost/program_options.hpp>
#include "thesisUtilities.h"
#include "opencvkeys.h"

using namespace cv;
using namespace std;
namespace ut = thesisUtilities;
namespace po = boost::program_options;

#define C_SELECT_CLICK_DISTANCE 10.0

// Global variables (need to be accessible by callbacks)
int f, xsize, ysize, n_frames;
vector<int> view_label_track;
vector<ut::heartPresent_t> heart_present_track;
int active_s, n_structures;
bool overwrite_mode;
vector<Mat> I;
Mat disp;
vector<string> structure_names;
vector<bool> touched;
vector<ut::subStructLabel_t> current_sl;
const std::string view_strings[4] = {std::string("BACKGROUND"),std::string("4-CHAM"),std::string("LVOT"),std::string("3V")};
const cv::Scalar view_colours[4] = {Scalar(255,255,255),Scalar(255,255,0), Scalar(0,255,0), Scalar(0,255,255)};
const cv::Scalar view_highlight_colours[4] = {Scalar(0,0,255),Scalar(0,127,255), Scalar(255,0,255), Scalar(255,0,0)};
const int n_views = 4;

// Function to display the current frame with the current annotations
void render()
{
	// Display the image
	disp = I[f].clone();

	for (int s = 0; s < n_structures; ++s)
	{
		Scalar colour = (s == active_s) ? view_highlight_colours[view_label_track[f]] : view_colours[view_label_track[f]];

		if(current_sl[s].present == ut::hpObscured)
			colour *= 0.5;

		if(current_sl[s].present != ut::hpNone)
		{
			const int lineThickness = 2;
			const int lineLength = 15;
			arrowedLine(disp,Point(current_sl[s].x,current_sl[s].y), Point(std::round(current_sl[s].x+lineLength*std::cos(float(current_sl[s].ori)*M_PI/180.0)),std::round(current_sl[s].y-lineLength*std::sin(float(current_sl[s].ori)*M_PI/180.0))),colour,lineThickness,8,0,0.2);
		}
	}
	string name_display_str = to_string(active_s) + string(": ") + structure_names[active_s];
	if(current_sl[active_s].present == ut::hpNone)
		name_display_str = string("(") + name_display_str + string(")");
	putText(disp,name_display_str,Point(5,15),FONT_HERSHEY_PLAIN,1.0,Scalar(0,255,255));

	if(heart_present_track[f] == ut::hpPresent || heart_present_track[f] == ut::hpObscured)
		putText(disp,view_strings[view_label_track[f]],Point(5,30),FONT_HERSHEY_PLAIN,1.0,view_colours[view_label_track[f]]);
	else
		putText(disp,string("-"),Point(5,30),FONT_HERSHEY_PLAIN,1.0,view_colours[0]);

	// Frame number display
	putText(disp,string("Frame ") + to_string(f) + string("/") + to_string(n_frames-1),Point(5,ysize-10),FONT_HERSHEY_PLAIN,1.0,Scalar(0,255,255));

	// Overwrite mode display
	if(overwrite_mode)
		putText(disp,"OVERWRITE",Point(xsize-100,ysize-10),FONT_HERSHEY_PLAIN,1.0,Scalar(0,255,255));

	imshow("Substructure Annotation",disp);
}



// Mouse callback function -- allows selecting, moving and rotating annotations
static void onMouse( int event, int x, int y, int /*flags*/, void* /*userdata*/)
{
	if( event == EVENT_LBUTTONDOWN )
	{
		// Move currently selected annotation
		current_sl[active_s].x = x;
		current_sl[active_s].y = y;
		touched[active_s] = true;
		if(current_sl[active_s].present == ut::hpNone)
			current_sl[active_s].present = ut::hpPresent;
	}
	else if (event == EVENT_RBUTTONDOWN)
	{
		// Rotate the currently selected annotation to point
		// towards the clicked poit
		current_sl[active_s].ori = std::atan2(current_sl[active_s].y-y,x-current_sl[active_s].x)*(180.0/M_PI);
		touched[active_s] = true;
		if(current_sl[active_s].present == ut::hpNone)
			current_sl[active_s].present = ut::hpPresent;
	}
	else if (event == EVENT_MBUTTONDOWN)
	{
		double shortest_distance = 1e10;
		int best_match = n_structures;
		// Select a new substructure if the click is close to one
		for (int s = 0; s < n_structures; ++s)
		{
			double distance = std::hypot(current_sl[s].x - x, current_sl[s].y - y);
			if((distance < C_SELECT_CLICK_DISTANCE) && (distance < shortest_distance))
			{
				shortest_distance = distance;
				best_match = s;
			}
		}
		if(best_match != n_structures)
			active_s = best_match;
		else
			return; // without rendering
	}
	else // return without rendering
		return;
	render();
}



int main(int argc, char** argv)
{
	int nextf, previousf = -1;
	float frame_rate;
	int keyPress = 0;
	VideoCapture vid_obj;
	bool irrelevant_key, exit_flag, read_error = false, read_success = false, record_mode = false, motion_prediction = true;
	VideoWriter output_video;
	string outvidname, trackdir, hearttrackdir, vidname, structfilename;

	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "produce help message")
		("video,v", po::value<string>(&vidname), "input video file")
		("structure_file,s", po::value<string>(&structfilename)->default_value("structures"), "file containing list of structures to annotate")
		("trackdirectory,t", po::value<string>(&trackdir)->default_value("./"), "directory containing the input/output track file")
		("hearttrackdirectory,d", po::value<string>(&hearttrackdir)->default_value("./"), "directory containing the relevant track file")
		("record,r" , "record the visualisation in a video file");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help"))
	{
		cout << "Video annotation tool for ultrasound fetal heart videos" << endl;
		cout << desc << endl;
		return 1;
	}

	if (vm.count("record"))
		record_mode = true;

	// Append trailing slash if necessary
	if(trackdir.at(trackdir.length() -1 ) != '/')
	trackdir += '/';

	// Open
	vid_obj.open(vidname);
	if ( !vid_obj.isOpened())
	{
		cout  << "Could not open reference " << vidname << endl;
		return -1;
	}

	xsize = vid_obj.get(CV_CAP_PROP_FRAME_WIDTH);
	ysize = vid_obj.get(CV_CAP_PROP_FRAME_HEIGHT);
	n_frames = vid_obj.get(CV_CAP_PROP_FRAME_COUNT);
	frame_rate = vid_obj.get(CV_CAP_PROP_FPS);

	// Read in all frames into a buffer (this gets around decoding issues)
	I.resize(n_frames);
	for(f = 0; f < n_frames; f++)
	{
		vid_obj >> I[f];

		// Occasionally the number of frames detected by opencv is wrong
		// Therefore check for empty frames
		if( I[f].rows <= 0)
		{
			n_frames = f;
			break;
		}
	}

    cout << "Heart Substructures Annotation Tool \n"
			"Control List: \n"
			"  Arrow Keys    : Move substructure (shift increases speed)\n"
			"  A/C           : Rotate substructure anticlockwise/clockwise (shift increases speed)\n"
			"  Delete        : Mark as present/not present/obscured (toggle) \n"
			"  Enter         : Move to next frame and save label \n"
			"  Backspace     : Move to previous frame and save label \n"
			"  </>           : Cycle forward/backward through substructures\n"
			"  Left-click    : Move substructure to location\n"
			"  Right-click   : Rotate substructure to point towards location\n"
			"  Middle-click  : Select nearby substructure\n"
			"  O             : Toggle overwrite mode (changes are propagated even to frames with existing labels) \n"
			"  P             : Move to the next frame without saving label \n"
			"  R             : Move to the previous frame without saving label \n"
			"  M             : Toggle motion prediction \n"
			"  Esc           : Exit (and save annotations) \n"
			"  Q             : Quit (discarding annotations) \n";
	cout << endl;

	// Occasionally OpenCV fails to find the frame rate, need to get the user to provide one
	if(isnan(frame_rate))
	{
		frame_rate = ut::getFrameRate(vidname,vidname.substr(0,vidname.find_last_of("/")));
		if(isnan(frame_rate))
		{
			cout << "OpenCV cannot automatically determine the frame rate, please manually provide one (in frames per second): " << endl << ">> " ;
			while(!(cin >> frame_rate)) {cin.clear(); cin.ignore(); cout << ">> "; }
		}
	}
	cout << "Using frame rate: " << frame_rate << endl;

	// Read in structures to label
	ifstream structfile(structfilename.c_str());
	vector<vector<int>> views_per_structure;
	if(structfile.is_open())
	{
		for(string linestring; !getline(structfile,linestring).eof() && !linestring.empty(); )
		{
			stringstream ss(linestring);
			string namestring;
			ss >> namestring;
			structure_names.emplace_back(namestring);
			views_per_structure.emplace_back(vector<int>());

			bool systole_only;
			ss >> systole_only; // FIXME currently not doing anything with this

			for(int v; ss >> v; )
				views_per_structure[structure_names.size()-1].emplace_back(v);
		}
	}
	else
	{
		cerr << "ERROR: Could not open structure file " << structfilename << endl;
		return EXIT_FAILURE;
	}
	n_structures = structure_names.size();

	// Vector listing the structures that are present in each view
	vector<vector<int>> structuresPerView(n_views);
	for(int s = 0; s < n_structures; ++s)
		for(int v : views_per_structure[s])
			structuresPerView[v].emplace_back(s);

	vector<vector<ut::subStructLabel_t>> track(n_frames, vector<ut::subStructLabel_t>(n_structures)) ;

	// Look for an existing track file
	string outfilename = vidname;
	string hearttrackfilename;
	outfilename.erase(outfilename.find_last_of("."), string::npos);
	outfilename.erase(0,outfilename.find_last_of("/")+1);
	outvidname = trackdir + outfilename + "_labels.avi";
	hearttrackfilename = hearttrackdir + outfilename + ".tk";
	outfilename = trackdir + outfilename + ".stk";

	// Open it to check it exists (there may be a better way to do this...)
	ifstream infile(outfilename.c_str());
	if (infile.is_open())
	{
		infile.close(); // close it to let the dedicated function read it
		read_success = ut::readSubstructuresTrackFile(outfilename, n_frames, structure_names, track);
		read_error = !read_success;
	}
	else
		read_success = false;

	if(read_error)
	{
		cerr << "Error reading existing trackfile " << outfilename << ", file will be ignored and overwritten (quit to retain this file)" << endl;
	}

	// Also get the view label information from the heart track file

	ifstream htfile(hearttrackfilename.c_str());
	if (htfile.is_open())
	{
		htfile.close(); // close it to let the dedicated function read it

		view_label_track.resize(n_frames); heart_present_track.resize(n_frames);
		int radius; bool headup;
		vector<bool> labelled_track(n_frames); vector<int> centrey_track(n_frames); vector<int> centrex_track(n_frames);
		vector<int> ori_track(n_frames); vector<int> phase_point_track(n_frames); vector<float> cardiac_phase_track(n_frames);

		if(!ut::readTrackFile(hearttrackfilename, n_frames, headup, radius, labelled_track, heart_present_track, centrey_track,
			                       centrex_track, ori_track, view_label_track, phase_point_track, cardiac_phase_track) )
		{
			cerr << "Could not read heart track information from " << hearttrackfilename << endl;
			return EXIT_FAILURE;
		}
	}
	else
	{
		cerr << "Could not read heart track information from " << hearttrackfilename << endl;
		return EXIT_FAILURE;
	}

	// Set frames where the heart is not present to the background class
	for(int g = 0; g < n_frames; ++g)
		if(heart_present_track[g] == ut::hpNone)
			view_label_track[g] = 0;

	if(read_error)
	{
		cerr << "Error reading existing trackfile " << outfilename << ", file will be ignored and overwritten (quit to retain this file)" << endl;
	}

	// Create a window and bind the mouse callback to it
	namedWindow( "Substructure Annotation", WINDOW_AUTOSIZE );// Create a window for display.
	setMouseCallback( "Substructure Annotation", onMouse, 0 );

	// Create an output video
	if(record_mode)
	{
		int ex = static_cast<int>(vid_obj.get(CV_CAP_PROP_FOURCC));
		output_video.open(outvidname, ex, frame_rate, Size(xsize,ysize), true);

		if (!output_video.isOpened())
		{
			cerr  << "Could not open the output video for write: " << outvidname << endl;
			return -1;
		}
	}

	// This will hold the current annotations
	vector<bool> just_stored_label(n_structures,false);
	touched.resize(n_structures,false);
	current_sl.resize(n_structures);

	// Loop through frames
	active_s = structuresPerView[view_label_track[0]][0];
	int active_s_view_specific_index = 0;
	exit_flag = false;
	f = 0;
	overwrite_mode = false;
	while(!exit_flag)
	{
		// Initialise the labels to this frame to either their previously labelled values
		// Or, if the frame has not been labelled yet, to the values from the previous frame
		// Start in the middle for the first frame
		fill(touched.begin(),touched.end(),false);

		// Select a new structure if the view has changed
		if(view_label_track[f] != view_label_track[previousf])
		{
			active_s_view_specific_index = 0;
			active_s = structuresPerView[view_label_track[f]][active_s_view_specific_index];
		}

		// Calculate a motion offset to use to estimate new positions
		Mat_<Vec2f> flow;
		if(motion_prediction && previousf >= 0 && any_of(just_stored_label.cbegin(),just_stored_label.cend(), [](bool b){return b;}) )
		{
			Mat oldim, newim;
			cvtColor(I[previousf],oldim,CV_BGR2GRAY);
			cvtColor(I[f],newim,CV_BGR2GRAY);
			calcOpticalFlowFarneback(oldim,newim,flow,0.5/*PYR_SCALE*/,3/*LEVELS*/,30/*WINSIZE*/,3/*ITERATIONS*/,7/*POLY_N*/,1.5/*POLY_SIGMA*/,OPTFLOW_FARNEBACK_GAUSSIAN/*FLAGS*/);
		}
		for (int s = 0; s < n_structures; ++s)
		{
			if((f == 0) && !track[0][s].labelled)
			{
				if(any_of(views_per_structure[s].cbegin(),views_per_structure[s].cend(),[](int v){return v == view_label_track[f];}))
				{
					current_sl[s].x = xsize-20;
					current_sl[s].y = 5+5*s;
					current_sl[s].ori = 0;
					current_sl[s].present = ut::hpPresent;
					touched[s] = true;
				}
				else
				{
					current_sl[s].x = -1;
					current_sl[s].y = -1;
					current_sl[s].ori = 0;
					current_sl[s].present = ut::hpNone;
				}
			}
			else
			{
				// Retrieve a previous labelling if one exists
				if( track[f][s].labelled && (!overwrite_mode || (overwrite_mode && !just_stored_label[s]) ) )
				{
					current_sl[s].x = track[f][s].x;
					current_sl[s].y = track[f][s].y;
					current_sl[s].ori = track[f][s].ori;
					current_sl[s].present = track[f][s].present;
				}
				// Propagate the label in the previously labelled frame
				else if(just_stored_label[s] && (view_label_track[f] == view_label_track[previousf]) )
				{
					if(motion_prediction && (previousf >= 0) && (track[previousf][s].x >= 0) && (track[previousf][s].y >= 0) && (track[previousf][s].x < xsize) && (track[previousf][s].y < ysize) )
					{
						Vec2f flow_offset = flow(track[previousf][s].y,track[previousf][s].x);
						current_sl[s].x = track[previousf][s].x + std::round(flow_offset[0]);
						current_sl[s].y = track[previousf][s].y + std::round(flow_offset[1]);
					}
					else
					{
						current_sl[s].x = track[previousf][s].x;
						current_sl[s].y = track[previousf][s].y;
					}
					current_sl[s].ori = track[previousf][s].ori;
					current_sl[s].present = track[previousf][s].present;
					touched[s] = true;
				}
				// Apply a default labelling
				else
				{
					if(any_of(views_per_structure[s].cbegin(),views_per_structure[s].cend(),[](int v){return v == view_label_track[f];}))
					{
						current_sl[s].x = xsize-4*s;
						current_sl[s].y = 20;
						current_sl[s].ori = 0;
						current_sl[s].present = ut::hpPresent;
					}
					else
					{
						current_sl[s].x = -1;
						current_sl[s].y = -1;
						current_sl[s].ori = 0;
						current_sl[s].present = ut::hpNone;
					}
				}
			}
		}

		fill(just_stored_label.begin(),just_stored_label.end(), false);
		nextf = f;
		while((nextf == f) && (!exit_flag))
		{
			// Display the current frame
			render();

			// Add this frame to the output video and continue to the next frame
			if(record_mode)
			{
				output_video << disp;
				nextf = f + 1;
				waitKey(10);
				break;
			}

			// Wait for a (relevant) key press
			do
			{
				irrelevant_key = false;
				keyPress = waitKey(0);
				switch(keyPress)
				{
					case LEFT_ARROW:
						if(current_sl[active_s].x > 0)
							current_sl[active_s].x--;
						touched[active_s] = true;
						if(current_sl[active_s].present == ut::hpNone)
							current_sl[active_s].present = ut::hpPresent;
						break;

					case RIGHT_ARROW:
						if(current_sl[active_s].x + 1 < xsize)
							current_sl[active_s].x++;
						touched[active_s] = true;
						if(current_sl[active_s].present == ut::hpNone)
							current_sl[active_s].present = ut::hpPresent;
						break;

					case UP_ARROW:
						if(current_sl[active_s].y > 0)
							current_sl[active_s].y--;
						touched[active_s] = true;
						if(current_sl[active_s].present == ut::hpNone)
							current_sl[active_s].present = ut::hpPresent;
						break;

					case DOWN_ARROW:
						if(current_sl[active_s].y + 1 < ysize)
							current_sl[active_s].y++;
						touched[active_s] = true;
						if(current_sl[active_s].present == ut::hpNone)
							current_sl[active_s].present = ut::hpPresent;
						break;

					case SHIFT_LEFT_ARROW:
						if(current_sl[active_s].x - 10 >= 0)
							current_sl[active_s].x -= 10;
						else
							current_sl[active_s].x = 0;
						touched[active_s] = true;
						if(current_sl[active_s].present == ut::hpNone)
							current_sl[active_s].present = ut::hpPresent;
						break;

					case SHIFT_RIGHT_ARROW:
						if(current_sl[active_s].x + 10 < xsize)
							current_sl[active_s].x += 10;
						else
							current_sl[active_s].x = xsize-1;
						touched[active_s] = true;
						if(current_sl[active_s].present == ut::hpNone)
							current_sl[active_s].present = ut::hpPresent;
						break;

					case SHIFT_UP_ARROW:
						if(current_sl[active_s].y - 10 >= 0)
							current_sl[active_s].y -= 10;
						else
							current_sl[active_s].y = 0;
						touched[active_s] = true;
						if(current_sl[active_s].present == ut::hpNone)
							current_sl[active_s].present = ut::hpPresent;
						break;

					case SHIFT_DOWN_ARROW:
						if(current_sl[active_s].y + 10 < ysize)
							current_sl[active_s].y += 10;
						else
							current_sl[active_s].y = ysize-1;
						touched[active_s] = true;
						if(current_sl[active_s].present == ut::hpNone)
							current_sl[active_s].present = ut::hpPresent;
						break;

					case A_KEY:
						current_sl[active_s].ori++;
						current_sl[active_s].ori %= 360;
						touched[active_s] = true;
						break;

					case C_KEY:
						current_sl[active_s].ori--;
						current_sl[active_s].ori %= 360;
						touched[active_s] = true;
						break;

					case SHIFT_A_KEY:
						current_sl[active_s].ori += 10;
						current_sl[active_s].ori %= 360;
						touched[active_s] = true;
						break;

					case SHIFT_C_KEY:
						current_sl[active_s].ori -= 10;
						current_sl[active_s].ori %= 360;
						touched[active_s] = true;
						break;

					case LESSTHAN_KEY:
						if(active_s_view_specific_index == 0)
							active_s_view_specific_index = structuresPerView[view_label_track[f]].size()-1;
						else
							active_s_view_specific_index--;
						active_s = structuresPerView[view_label_track[f]][active_s_view_specific_index];
						break;

					case MORETHAN_KEY:
						active_s_view_specific_index++;
						active_s_view_specific_index %= structuresPerView[view_label_track[f]].size();
						active_s = structuresPerView[view_label_track[f]][active_s_view_specific_index];
						break;

					case ZERO_KEY:
					case ONE_KEY:
					case TWO_KEY:
					case THREE_KEY:
					case FOUR_KEY:
					case FIVE_KEY:
					case SIX_KEY:
					case SEVEN_KEY:
					case EIGHT_KEY:
					case NINE_KEY:
						if(keyPress - ZERO_KEY < int(structuresPerView[view_label_track[f]].size()))
						{
							active_s_view_specific_index = keyPress - ZERO_KEY ;
							active_s = structuresPerView[view_label_track[f]][active_s_view_specific_index];
						}
						else
							irrelevant_key = true;
						break;

					case O_KEY:
						overwrite_mode = !overwrite_mode;
						break;

					case M_KEY:
						motion_prediction = !motion_prediction;
						break;

					case P_KEY:
					case RETURN_KEY:
					case VAR_RETURN_KEY:
						nextf = f + 1;
						break;

					case R_KEY:
					case BACKSPACE_KEY:
					case VAR_BACKSPACE_KEY:
						if(f == 0)
							irrelevant_key = true; // ignore command to go backwards when at the start of the video
						else
							nextf = f - 1;
						break;

					case ESC_KEY:
						exit_flag = true;
						break;

					case Q_KEY:
						exit_flag = true;
						break;

					case DELETE_KEY:
						current_sl[active_s].present = (current_sl[active_s].present+1)%3;
						touched[active_s] = true;
						break;

					default:
						irrelevant_key = true;
						break;

				} // switch

			} while(irrelevant_key);

		}

		if( (keyPress == RETURN_KEY) || (keyPress == VAR_RETURN_KEY) || (keyPress == BACKSPACE_KEY) || (keyPress == VAR_BACKSPACE_KEY))
		{
			for (int s = 0; s < n_structures; ++s)
			{
				// Check to see whether the annotations for this substructure have actually changed
				if(touched[s])
				{
					track[f][s].x = current_sl[s].x;
					track[f][s].y = current_sl[s].y;
					track[f][s].ori = current_sl[s].ori;
					track[f][s].present = current_sl[s].present;
					track[f][s].labelled = true;
					just_stored_label[s] = true;
				}
				else if(track[f][s].labelled == true)
					just_stored_label[s] = true;
			}
		}

		if(f == n_frames - 1 )
		{
			if( (keyPress == RETURN_KEY) || (keyPress == VAR_RETURN_KEY) || record_mode )
				exit_flag = true;
			else if (keyPress == P_KEY)
				nextf = f; // stall on the final frame
		}

		// Decide where to go next
		previousf = f;
		f = nextf;


	} // frame loop

	// Write to file
	if( (keyPress != Q_KEY) && (!record_mode) )
	{
		ofstream outfile(outfilename.c_str());
		if (!outfile.is_open())
			return false;

		outfile << "# frame_no labelled present y x orientation" << endl;
		outfile << " " << n_structures << " " << xsize << " " << ysize << endl << endl;

		for (int s = 0; s < n_structures; ++s)
		{
			outfile << s << " " << structure_names[s] << endl;
			for(f = 0; f < n_frames; f++)
			{
				// Stipulate that substructures must be obscured if the whole heart is obscured
				if(heart_present_track[f] == ut::hpObscured && track[f][s].present == ut::hpPresent && std::none_of(views_per_structure[s].cbegin(),views_per_structure[s].cend(),[](int v){return v == 0;}) )
					track[f][s].present = ut::hpObscured;

				// Also ensure that any lablled locations that are off the edge of the image are
				// marked as not present
				if( (track[f][s].y >= ysize) || (track[f][s].y < 0) || (track[f][s].x >= xsize) || (track[f][s].x < 0) )
					track[f][s].present = ut::hpNone;

				outfile << f << " "
						<< track[f][s].labelled << " "
						<< track[f][s].present << " "
						<< track[f][s].y << " "
						<< track[f][s].x << " "
						<< track[f][s].ori <<
						endl;
			}
			outfile << endl;
		}
		outfile.close();
	}

	if(record_mode)
		output_video.release();

}
