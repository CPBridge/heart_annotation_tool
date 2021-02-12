#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "thesisUtilities.h"
#include "opencvkeys.h"

using namespace cv;
using namespace std;
namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace ut = thesisUtilities;

// View Codes
#define VIEW_4CHAM 1
#define VIEW_LVOT 2
#define VIEW_RVOT 3
#define VIEW_VSIGN 4

// View Keys
#define CHAM4_KEY ONE_KEY
#define LVOT_KEY TWO_KEY
#define RVOT_KEY THREE_KEY
#define VSIGN_KEY FOUR_KEY

// View Strings
#define STR_4CHAM "4-CHAM"
#define STR_LVOT "LVOT"
#define STR_RVOT "3V"
#define STR_VSIGN "V-SIGN"

// View Colours
#define CLR_4CHAM Scalar(255,255,0) // cyan
#define CLR_LVOT Scalar(0,255,0) // green
#define CLR_RVOT Scalar(0,255,255) // yellow
#define CLR_VSIGN Scalar(0,0,255)  // red

// Labellings for cardiac phase points
#define NOT_LABELLED 0
#define AUTO_LABELLED_SYSTOLE 1
#define MANUALLY_LABELLED_SYSTOLE 2
#define AUTO_LABELLED_DIASTOLE 3
#define MANUALLY_LABELLED_DIASTOLE 4

// Typical fetal heart rates (BPM)
#define MIN_HEART_RATE 110.0
#define MAX_HEART_RATE 160.0

// Prototypes
// Function to update the cardiac phase labels
bool recalculate_cardiac_phase(int n_frames, float &cardiac_period, float frame_rate, float *cardiac_phase_track,int *phase_point_track);

int main(int argc, char** argv)
{
	int f, nextf, previousf = -1, centrex, centrey, radius = 80, ori, view_label, phase_point;
	ut::heartPresent_t heart_present;
	bool headup = true;
	float cardiac_period, cardiac_phase, frame_rate;
	int xsize, ysize, n_frames;
	int key_press = -1;
	Mat disp;
	vector<Mat> I;
	VideoCapture vid_obj;
	bool irrelevant_key, exit_flag, overwrite_mode = false, read_error = false, read_success = false, record_mode = false,
		 just_stored_label = false, cardiac_phase_valid = false;
	Scalar colour;
	string view_string;
	VideoWriter output_video;
	fs::path trackdir, vidname;

	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "produce help message")
		("video,v", po::value<fs::path>(&vidname), "input video file")
		("trackdirectory,t", po::value<fs::path>(&trackdir)->default_value("."), "directory containing the input/output track file")
		("record,r" , "record the visualisation in a video file");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help"))
	{
		cout << "Video annotation tool for ultrasound fetal heart videos" << endl;
		cout << desc << endl;
		return EXIT_SUCCESS;
	}

	if (vm.count("record"))
		record_mode = true;

	// Open
	vid_obj.open(vidname.string());
	if ( !vid_obj.isOpened())
	{
		cerr  << "Could not open reference " << vidname << endl;
		return EXIT_FAILURE;
	}

	xsize = vid_obj.get(cv::CAP_PROP_FRAME_WIDTH);
	ysize = vid_obj.get(cv::CAP_PROP_FRAME_HEIGHT);
	n_frames = vid_obj.get(cv::CAP_PROP_FRAME_COUNT);
	frame_rate = vid_obj.get(cv::CAP_PROP_FPS);

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

	cout << "Heart Annotation Tool \n"
			"Control List: \n"
			"  Arrow Keys : Move heart centre \n"
			"  +/-        : Increase/decrease radius  \n"
			"  A/C        : Rotate heart anticlockwise/clockwise \n"
			"  H          : Toggle head up / head down \n"
			"  1          : Mark as 4CHAM view\n"
			"  2          : Mark as LVOT view\n"
			"  3          : Mark as 3V view\n"
			"  4          : Mark as VSIGN view\n"
			"  S          : Mark frame as end-systole (toggle) \n"
			"  D          : Mark frame as end-diastole (toggle) \n"
			"  Z          : Automatically (re-)calculate cardiac phase values \n"
			"  Delete     : Marked as present/not present/obscured (toggle) \n"
			"  Enter      : Move to next frame and save label \n"
			"  Backspace  : Move to previous frame and save label \n"
			"  O          : Toggle overwrite mode (changes are propagated even to frames with existing labels) \n"
			"  P          : Move to the next frame without saving label \n"
			"  R          : Move to the previous frame without saving label \n"
			"  Esc        : Exit (and save annotations) \n"
			"  Q          : Quit (discarding annotations) \n";
	cout << endl;

	// Occasionally OpenCV fails to find the frame rate, need to get the user to provide one
	if(isnan(frame_rate))
	{
		frame_rate = ut::getFrameRate(vidname.string(),vidname.parent_path().string());
		if(isnan(frame_rate))
		{
			cout << "OpenCV cannot automatically determine the frame rate, please manually provide one (in frames per second): " << endl << ">> " ;
			while(!(cin >> frame_rate)) {cin.clear(); cin.ignore(); cout << ">> "; }
		}
	}
	cout << "Using frame rate: " << frame_rate << endl;

	// Create tracks
	vector<int> centrex_track(n_frames);
	vector<int> centrey_track(n_frames);
	vector<int> ori_track(n_frames);
	vector<int> view_label_track(n_frames);
	vector<ut::heartPresent_t> heart_present_track(n_frames);
	vector<int> phase_point_track(n_frames);
	vector<bool> labelled_track(n_frames);
	vector<float> cardiac_phase_track(n_frames);

	// Look for an existing track file
	const fs::path outvidname = trackdir / vidname.stem().concat("_labels").replace_extension(".avi");
	const fs::path outfilename = trackdir / vidname.stem().replace_extension(".tk");

	// Check it exists
	if(fs::exists(outfilename))
	{
		read_success = ut::readTrackFile(outfilename.string(), n_frames, headup, radius, labelled_track, heart_present_track, centrey_track,
			                       centrex_track, ori_track, view_label_track, phase_point_track, cardiac_phase_track);
		read_error = !read_success;
	}
	else
		read_success = false;

	if(read_error)
	{
		cerr << "Error reading existing trackfile " << outfilename << ", file will be ignored and overwritten (quit to retain this file)" << endl;
	}

	if(!read_success)
	{
		// Initialise tracks as -1 (unlabelled)
		for(f = 0 ; f < n_frames; f++)
		{
			labelled_track[f] = false;
			phase_point_track[f] = NOT_LABELLED;
			cardiac_phase_track[f] = -1.0;
		}
	}
	else if(  (cardiac_phase_track[0] >= 0.0) && (labelled_track[0]) )
		cardiac_phase_valid = true;

	namedWindow( "Heart Annotation", WINDOW_AUTOSIZE );// Create a window for display.


	// Create an output video
	if(record_mode)
	{
		int ex = static_cast<int>(vid_obj.get(cv::CAP_PROP_FOURCC));
		output_video.open(outvidname.string(), ex, frame_rate, Size(xsize,ysize), true);

		if (!output_video.isOpened())
		{
			cerr  << "Could not open the output video for write: " << outvidname << endl;
			return EXIT_FAILURE;
		}
	}

	// Loop through frames
	exit_flag = false;
	f = 0;
	while(!exit_flag)
	{
		// Initialise the labels to this frame to either their previously labelled values
		// Or, if the frame has not been labelled yet, to the values from the previous frame
		// Start in the middle for the first frame
		if((f == 0) && (!labelled_track[0]))
		{
			centrex = xsize/2;
			centrey = ysize/2;
			ori = 90;
			view_label = VIEW_4CHAM;
			heart_present = ut::hpPresent;
			phase_point = NOT_LABELLED;
			cardiac_phase = -1.0;
		}
		else
		{
			// Retrieve a previous labelling if one exists
			if( labelled_track[f] && (!overwrite_mode || (overwrite_mode && !just_stored_label) ) )
			{
				centrex = centrex_track[f];
				centrey = centrey_track[f];
				ori = ori_track[f];
				view_label = view_label_track[f];
				heart_present = heart_present_track[f];
				phase_point = phase_point_track[f];
			}
			// Propagate the label in the previously labelled frame
			else if(just_stored_label)
			{
				centrex = centrex_track[previousf];
				centrey = centrey_track[previousf];
				ori = ori_track[previousf];
				view_label = view_label_track[previousf];
				heart_present = heart_present_track[previousf];
				phase_point = NOT_LABELLED; // don't propogate this...
			}
			// Apply a default labelling
			else
			{
				centrex = xsize/2;
				centrey = ysize/2;
				ori = 90;
				view_label = VIEW_4CHAM;
				heart_present = ut::hpPresent;
				phase_point = cardiac_phase_valid ? phase_point_track[f]  : NOT_LABELLED;
			}

			if(cardiac_phase_valid)
				cardiac_phase = cardiac_phase_track[f];
			else
				cardiac_phase = -1.0;
		}

		just_stored_label = false;
		nextf = f;
		while((nextf == f) && (!exit_flag))
		{
			// Display the image
			disp = I[f].clone();

			switch(view_label)
			{
				case VIEW_4CHAM:
					colour = CLR_4CHAM;
					view_string = STR_4CHAM;
					break;
				case VIEW_LVOT:
					colour = CLR_LVOT;
					view_string = STR_LVOT;
					break;
				case VIEW_RVOT:
					colour = CLR_RVOT;
					view_string = STR_RVOT;
					break;
				case VIEW_VSIGN:
					colour = CLR_VSIGN;
					view_string = STR_VSIGN;
					break;
			}

			if(heart_present == ut::hpObscured)
				colour *= 0.5;

			if(heart_present != ut::hpNone)
			{
				int lineThickness = 2;
				circle(disp,Point(centrex,centrey),radius,colour,lineThickness);
				line(disp,Point(centrex,centrey), Point(std::round(centrex+radius*std::cos(float(ori)*M_PI/180.0)),std::round(centrey-radius*std::sin(float(ori)*M_PI/180.0))),colour,lineThickness);
				if(headup)
				{
					putText(disp,"L",Point(std::round(centrex+(radius+10)*std::cos(float(ori+90)*M_PI/180.0))-5,std::round(centrey-(radius+10)*std::sin(float(ori+90)*M_PI/180.0))+5),FONT_HERSHEY_PLAIN,1.0,colour);
					putText(disp,"R",Point(std::round(centrex+(radius+10)*std::cos(float(ori-90)*M_PI/180.0))-5,std::round(centrey-(radius+10)*std::sin(float(ori-90)*M_PI/180.0))+5),FONT_HERSHEY_PLAIN,1.0,colour);
				}
				else
				{
					putText(disp,"R",Point(std::round(centrex+(radius+10)*std::cos(float(ori+90)*M_PI/180.0))-5,std::round(centrey-(radius+10)*std::sin(float(ori+90)*M_PI/180.0))+5),FONT_HERSHEY_PLAIN,1.0,colour);
					putText(disp,"L",Point(std::round(centrex+(radius+10)*std::cos(float(ori-90)*M_PI/180.0))-5,std::round(centrey-(radius+10)*std::sin(float(ori-90)*M_PI/180.0))+5),FONT_HERSHEY_PLAIN,1.0,colour);
				}
				putText(disp,view_string,Point(std::round(centrex+20*std::cos(float(ori+180)*M_PI/180.0))-30,std::round(centrey-20*std::sin(float(ori+180)*M_PI/180.0))+5),FONT_HERSHEY_PLAIN,1.0,colour);


				// Cardiac phase visualisation
				if(cardiac_phase_valid)
				{
					Point phaseVisPoint(std::round(centrex + 0.5*(1-std::cos(cardiac_phase))*radius*std::cos(float(ori)*M_PI/180.0)),std::round(centrey - 0.5*(1-std::cos(cardiac_phase))*radius*std::sin(float(ori)*M_PI/180.0)));
					circle(disp,phaseVisPoint,3,colour,-1);
					if (cardiac_phase < M_PI)
						arrowedLine(disp,phaseVisPoint,Point(std::round(phaseVisPoint.x + 10.0*std::cos(float(ori)*M_PI/180.0)),std::round(phaseVisPoint.y - 10.0*std::sin(float(ori)*M_PI/180.0))),colour,lineThickness,8,0,1.5);
					else
						arrowedLine(disp,phaseVisPoint,Point(std::round(phaseVisPoint.x - 10.0*std::cos(float(ori)*M_PI/180.0)),std::round(phaseVisPoint.y + 10.0*std::sin(float(ori)*M_PI/180.0))),colour,lineThickness,8,0,1.5);
				}
			}

			// End systole and end diastole labels
			if(phase_point == AUTO_LABELLED_SYSTOLE)
				putText(disp,"(ES)",Point(std::round(centrex+(radius+10)*std::cos(float(ori)*M_PI/180.0))-5,std::round(centrey-(radius+10)*std::sin(float(ori)*M_PI/180.0))+5),FONT_HERSHEY_PLAIN,1.0,colour);
			else if (phase_point == MANUALLY_LABELLED_SYSTOLE)
				putText(disp,"ES",Point(std::round(centrex+(radius+10)*std::cos(float(ori)*M_PI/180.0))-5,std::round(centrey-(radius+10)*std::sin(float(ori)*M_PI/180.0))+5),FONT_HERSHEY_PLAIN,1.0,colour);
			else if(phase_point == AUTO_LABELLED_DIASTOLE)
				putText(disp,"(ED)",Point(std::round(centrex+(radius+10)*std::cos(float(ori)*M_PI/180.0))-5,std::round(centrey-(radius+10)*std::sin(float(ori)*M_PI/180.0))+5),FONT_HERSHEY_PLAIN,1.0,colour);
			else if(phase_point == MANUALLY_LABELLED_DIASTOLE)
				putText(disp,"ED",Point(std::round(centrex+(radius+10)*std::cos(float(ori)*M_PI/180.0))-5,std::round(centrey-(radius+10)*std::sin(float(ori)*M_PI/180.0))+5),FONT_HERSHEY_PLAIN,1.0,colour);

			// Frame number display
			putText(disp,string("Frame ") + to_string(f) + string("/") + to_string(n_frames-1),Point(5,ysize-10),FONT_HERSHEY_PLAIN,1.0,Scalar(0,255,255));

			// Overwrite mode display
			if(overwrite_mode)
				putText(disp,"OVERWRITE",Point(xsize-100,ysize-10),FONT_HERSHEY_PLAIN,1.0,Scalar(0,255,255));

			imshow("Heart Annotation",disp);

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
				key_press = waitKey(0);
				switch(key_press)
				{
					case LEFT_ARROW:
						if(centrex - 1 - int(std::ceil(radius)) > 0)
							centrex--;
						break;

					case RIGHT_ARROW:
						if(centrex + 1 + int(std::ceil(radius)) < xsize)
							centrex++;
						break;

					case UP_ARROW:
						if(centrey - 1 - int(std::ceil(radius)) > 0)
							centrey--;
						break;

					case DOWN_ARROW:
						if(centrey + 1 + int(std::ceil(radius)) < ysize)
							centrey++;
						break;

					case PLUS_KEY:
						if( (centrex - int(std::ceil(radius)+1) > 0) &&
						  (centrex + int(std::ceil(radius)+1) < xsize) &&
						  (centrey - int(std::ceil(radius)+1) > 0) &&
						  (centrey + int(std::ceil(radius)+1) < ysize) )
							radius++;
						break;

					case MINUS_KEY:
						radius--;
						break;

					case A_KEY:
						ori++;
						ori %= 360;
						break;

					case C_KEY:
						ori--;
						ori %= 360;
						break;

					case H_KEY:
						headup = !headup;
						break;

					case CHAM4_KEY:
						view_label = VIEW_4CHAM;
						break;

					case LVOT_KEY:
						view_label = VIEW_LVOT;
						break;

					case RVOT_KEY:
						view_label = VIEW_RVOT;
						break;

					case VSIGN_KEY:
						view_label = VIEW_VSIGN;
						break;

					case O_KEY:
						overwrite_mode = !overwrite_mode;
						break;

					case S_KEY:
						if(phase_point == MANUALLY_LABELLED_SYSTOLE)
							phase_point = NOT_LABELLED;
						else
							phase_point = MANUALLY_LABELLED_SYSTOLE;
						break;

					case D_KEY:
						if(phase_point == MANUALLY_LABELLED_DIASTOLE)
							phase_point = NOT_LABELLED;
						else
							phase_point = MANUALLY_LABELLED_DIASTOLE;
						break;

					case Z_KEY:
						cardiac_phase_valid = recalculate_cardiac_phase(n_frames, cardiac_period, frame_rate, cardiac_phase_track.data(),phase_point_track.data());
						if(cardiac_phase_valid)
							cardiac_phase = cardiac_phase_track[f];
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
						switch(heart_present)
						{
							case ut::hpNone:
								heart_present = ut::hpPresent;
							break;
							case ut::hpPresent:
								heart_present = ut::hpObscured;
							break;
							case ut::hpObscured:
								heart_present = ut::hpNone;
							break;
						}
						break;

					default:
						irrelevant_key = true;
						break;

				} // switch

			} while(irrelevant_key);

		}

		if( (key_press == RETURN_KEY) || (key_press == VAR_RETURN_KEY) || (key_press == BACKSPACE_KEY) || (key_press == VAR_BACKSPACE_KEY))
		{
			// Store the values for the frame we just annotated
			centrex_track[f] = centrex;
			centrey_track[f] = centrey;
			view_label_track[f] = view_label;
			ori_track[f] = ori;
			heart_present_track[f] = heart_present;
			labelled_track[f] = true;
			phase_point_track[f] = phase_point;
			just_stored_label = true;
		}

		if(f == n_frames - 1 )
		{
			if( (key_press == RETURN_KEY) || (key_press == VAR_RETURN_KEY) || record_mode )
				exit_flag = true;
			else if (key_press == P_KEY)
				nextf = f; // stall on the final frame
		}

		// Decide where to go next
		previousf = f;
		f = nextf;

	} // frame loop

	// Write to file
	if( (key_press != Q_KEY) && (!record_mode) )
	{
		ofstream outfile(outfilename.string().c_str());
		if (!outfile.is_open())
			return false;

		outfile << "# frame_no labelled present centrey centrex orientation view_label phasepoints cardiac_phase" << endl;
		outfile << xsize << " " << ysize << endl;
		outfile << headup << " " << radius << endl;

		for(f = 0; f < n_frames; f++)
		{
			if(!labelled_track[f])
			{
				 heart_present_track[f] = ut::hpNone;
				 centrey_track[f] = 0;
				 centrex_track[f] = 0;
				 ori_track[f] = 0;
				 view_label_track[f] = 0;
			}

			outfile << f << " "
					<< labelled_track[f] << " "
					<< heart_present_track[f] << " "
					<< centrey_track[f] << " "
					<< centrex_track[f] << " "
					<< ori_track[f] << " "
					<< view_label_track[f] << " "
					<< phase_point_track[f] << " "
					<< cardiac_phase_track[f] <<
					endl;
		}


		outfile.close();
	}

	if(record_mode)
		output_video.release();

}


// Function to update the cardiac phase labels
bool recalculate_cardiac_phase(int n_frames, float &cardiac_period, float frame_rate, float *cardiac_phase_track,int *phase_point_track)
{
	int f, n;
	list<int> end_systole_frames, end_diastole_frames;
	list<int>::iterator sys_it, dias_it, sys_it2, dias_it2;
	int num_in_average = 0, first_labelled, last_labelled, number_to_add;
	float running_total_length = 0.0, min_frames_per_beat, max_frames_per_beat, beat_length, spacing;
	bool in_systole, video_end;

	// Calculate the minimum and maximum acceptable periods for a single heart beat
	min_frames_per_beat = 60.0*frame_rate/MAX_HEART_RATE;
	max_frames_per_beat = 60.0*frame_rate/MIN_HEART_RATE;


	// Loop through frames, adding up difference between successive end-systoles and end-diastoles
	for(f = 0; f < n_frames; f++)
	{
		if(phase_point_track[f] == MANUALLY_LABELLED_DIASTOLE)
		{
			if(end_diastole_frames.size() > 0)
			{
				beat_length = float(f - end_diastole_frames.back());
				if( (beat_length > min_frames_per_beat) && (beat_length < max_frames_per_beat) )
				{
					num_in_average++;
					running_total_length += beat_length;
				}
			}
			end_diastole_frames.emplace_back(f);
		}

		else if(phase_point_track[f] == MANUALLY_LABELLED_SYSTOLE)
		{
			if(end_systole_frames.size() > 0)
			{
				beat_length = float(f - end_systole_frames.back());
				if( (beat_length > min_frames_per_beat) && (beat_length < max_frames_per_beat) )
				{
					num_in_average++;
					running_total_length += beat_length;
				}
			}
			end_systole_frames.emplace_back(f);
		}

		// Remove any previous automated markers
		else
			phase_point_track[f] = NOT_LABELLED;
	}

	// Check that there are sufficiently many labelled points to actually predict other values
	if( (end_diastole_frames.size() < 1) || (end_systole_frames.size() < 1) || num_in_average == 0 )
	{
		cerr << "ERROR: You need to have labelled at least one end-systole and one end-diastole frame, and additionally one consecutive pair of either end-systole or end-diastole frames" << endl;
		return false;
	}

	// Calculate the average time period of the cardiac cycle
	cardiac_period = running_total_length/num_in_average;

	// Fill in end-systole frames before the first labelled one
	first_labelled = end_systole_frames.front();
	f = first_labelled - std::round(cardiac_period);
	n = 1;
	while(f >= 0)
	{
		end_systole_frames.push_front(f);
		phase_point_track[f] = AUTO_LABELLED_SYSTOLE;
		n++;
		f = std::round(float(first_labelled) - n*cardiac_period);
	}

	// Same for end-distole frames before the first labelled one
	first_labelled = end_diastole_frames.front();
	f = first_labelled - std::round(cardiac_period);
	n = 1;
	while(f >= 0)
	{
		end_diastole_frames.push_front(f);
		phase_point_track[f] = AUTO_LABELLED_DIASTOLE;
		n++;
		f = std::round(float(first_labelled) - n*cardiac_period);
	}

	// Now fill in end-systole frames after the final labelled one
	last_labelled = end_systole_frames.back();
	f = last_labelled + std::round(cardiac_period);
	n = 1;
	while(f < n_frames)
	{
		end_systole_frames.emplace_back(f);
		phase_point_track[f] = AUTO_LABELLED_SYSTOLE;
		n++;
		f = std::round(float(last_labelled) + n*cardiac_period);
	}

	// Now fill in end-diastole frames after the final labelled one
	last_labelled = end_diastole_frames.back();
	f = last_labelled + std::round(cardiac_period);
	n = 1;
	while(f < n_frames)
	{
		end_diastole_frames.emplace_back(f);
		phase_point_track[f] = AUTO_LABELLED_DIASTOLE;
		n++;
		f = std::round(float(last_labelled) + n*cardiac_period);
	}


	// Now add end_systole frames in gaps
	sys_it = end_systole_frames.begin();
	sys_it2 = next(sys_it);
	while(sys_it2 != end_systole_frames.end())
	{
		// If the difference between these pairs is greater than
		// the allowable distance, we need to add some frames in between
		if(*sys_it2 - *sys_it > max_frames_per_beat)
		{
			number_to_add = std::round(float(*sys_it2 - *sys_it)/cardiac_period) - 1;
			spacing = float(*sys_it2 - *sys_it)/float(number_to_add+1);
			// Insert into the vector, equally spaced
			for(n = 1; n <= number_to_add; n++)
			{
				f = *sys_it + std::round(n*spacing);
				phase_point_track[f] = AUTO_LABELLED_SYSTOLE;
				end_systole_frames.insert(sys_it2,f);
			}
		}
		// Advance iterators to the next pair
		sys_it = sys_it2;
		sys_it2++;
	}

	// Same for end diastole frames in gaps
	dias_it = end_diastole_frames.begin();
	dias_it2 = next(dias_it);
	while(dias_it2 != end_diastole_frames.end())
	{
		// If the difference between these pairs is greater than
		// the allowable distance, we need to add some frames in between
		if(*dias_it2 - *dias_it > max_frames_per_beat)
		{
			number_to_add = std::round(float(*dias_it2 - *dias_it)/cardiac_period) - 1;
			spacing = float(*dias_it2 - *dias_it)/float(number_to_add+1);
			// Insert into the vector, equally spaced
			for(n = 1; n <= number_to_add; n++)
			{
				f = *dias_it + std::round(n*spacing);
				phase_point_track[f] = AUTO_LABELLED_DIASTOLE;
				end_diastole_frames.insert(dias_it2,f);
			}
		}
		// Advance iterators to the next pair
		dias_it = dias_it2;
		dias_it2++;
	}

	// A third loop to predict the cardiac phase for all frames
	sys_it = end_systole_frames.begin();
	dias_it = end_diastole_frames.begin();
	if(*dias_it > *sys_it)
	{
		// The video starts during systole
		in_systole = true;
		// Place an imaginary end_diastole frame at the beginning
		end_diastole_frames.push_front(std::round(*dias_it - cardiac_period));
		dias_it = end_diastole_frames.begin();
	}
	else
	{
		// The video starts during diastole
		in_systole = false;
		// Place an imaginary end_systole frame at the beginning
		end_systole_frames.push_front(std::round(*sys_it - cardiac_period));
		sys_it = end_systole_frames.begin();
	}

	// Put in an imaginary frame at the end too
	if( end_diastole_frames.back() > end_systole_frames.back() )
		end_systole_frames.emplace_back(std::round( end_systole_frames.back() + cardiac_period));
	else
		end_diastole_frames.emplace_back(std::round( end_diastole_frames.back() + cardiac_period));

	// Check that the end diastole and end systole frames alternate as they should do
	bool last_was_end_systole = (end_systole_frames.front() > end_diastole_frames.front());
	list<int>::iterator sys_it_test = end_systole_frames.begin(), dias_it_test = end_diastole_frames.begin();
	while((sys_it_test != end_systole_frames.end()) || (dias_it_test != end_diastole_frames.end()))
	{
		if((sys_it_test != end_systole_frames.end()) && (dias_it_test != end_diastole_frames.end()))
		{
			if(*sys_it == *dias_it)
			{
				cout << "ERROR: The end diastole and systole frames you have chosen give rise to an inconsistency at around frame "
				<< *dias_it_test << ". Please carefully check your annotations and try again." << endl;
				return false;
			}
		}
		if(sys_it_test == end_systole_frames.end() || ( (dias_it_test != end_diastole_frames.end()) && (*sys_it_test > *dias_it_test) ) )
		{
			// cout << "       " << *dias_it_test << "" << endl; // Helpful debug output
			if(!last_was_end_systole)
			{
				cout << "ERROR: The end diastole and systole frames you have chosen give rise to an inconsistency at around frame "
				<< *dias_it_test << ". Please carefully check your annotations and try again." << endl;
				return false;
			}
			last_was_end_systole = false;
			++dias_it_test;
		}
		else
		{
			// cout << *sys_it_test << "" << endl; // Helpful debug output
			if(last_was_end_systole)
			{
				cout << "ERROR: The end diastole and systole frames you have chosen give rise to an inconsistency at around frame "
				<< *sys_it_test << ". Please carefully check your annotations and try again." << endl;
				return false;
			}
			last_was_end_systole = true;
			++sys_it_test;
		}
	}
	assert(sys_it_test == end_systole_frames.end() && dias_it_test == end_diastole_frames.end());

	f = 0;
	video_end = false;

	while(!video_end)
	{
		if(in_systole)
		{
			// Loop through frames before the end systole frame
			while(f < *sys_it)
			{
				if(f >= n_frames)
				{
					video_end = true;
					break;
				}
				cardiac_phase_track[f] = M_PI*float(f - *dias_it)/float(*sys_it - *dias_it);
				f++;
			}
			// Now we are at the end systole frame
			if(video_end || f >= n_frames)
				break;
			in_systole = false;
			cardiac_phase_track[f++] = M_PI;
			dias_it++;
		}
		else
		{
			// Loop through frames before the end diastole frame
			while(f < *dias_it)
			{
				if(f >= n_frames)
				{
					video_end = true;
					break;
				}
				cardiac_phase_track[f] = M_PI + M_PI*float(f - *sys_it)/float(*dias_it - *sys_it);
				f++;
			}
			// Now we are at the end diastole frame
			if(video_end || f >= n_frames)
				break;
			in_systole = true;
			cardiac_phase_track[f++] = 0.0;
			sys_it++;
		}
	}

	return true;

}
