import numpy as np
import math

# Constants for (heart) trackfile definition
tk_frameCol = 0
tk_labelledCol = 1
tk_presentCol = 2
tk_yposCol = 3
tk_xposCol = 4
tk_oriCol = 5
tk_viewCol = 6
tk_phasePointsCol = 7
tk_cardiacPhaseCol= 8

# Constants for substructure track file definition
stk_frameCol = 0
stk_labelledCol = 1
stk_presentCol = 2
stk_yposCol = 3
stk_xposCol = 4
stk_oriCol = 5

# Reads in the information in a heart track file and returns
def readHeartTrackFile(filename) :
	'''
	Read data from a heart track (.tk) file.

	Arguments:
	* filename -- String containing file path and name for the .tk file to read from

	Returns:
	* table -- numpy array containing the data for each frame.
	  Each row corresponds to a frame and each column corresponds to a variable.
	  The columns my be indexed by the variables tk_frameCol, tk_labelledCol,
	  tk_presentCol, tk_yposCol, tk_xposCol, tk_oriCol, tk_viewCol,
	  tk_phasePointsCol, tk_cardiacPhaseCol
	* image_dims -- 2-element list containing width and height of the video in
	  pixels
	* headup -- Boolean variable indicating the 'flip' of the video. True
	  indicates that the cross section is being viewed along the direction from
	  the fetal head to the fetal toes. False indicates the other direction.
	'''

	# Open the file to read info on the first couple of lines
	with open(filename) as infile :
		# Skip the first comment line
		infile.readline()

		# The next line contains the image dimensions
		image_dims = list(map(int,infile.readline().split()))

		# The next line contains the headup and radius information
		line_info = infile.readline().split()
		headup = bool(line_info[0])
		radius = float(line_info[1])

	# Now use loadtxt to read the rest of the data
	table = np.loadtxt(filename,skiprows=3)
	return table,image_dims,headup,radius



# Reads the track file and return as rows of dictionaries
def readHeartTrackFileAsDicts(filename) :
	'''
	Read data from a heart track (.tk) file and return the frame-wise
	annotations as a list of dictionaries.

	Arguments:
	* filename -- String containing file path and name for the .tk file to read from

	Returns:
	* dict_rows -- A list of dictionaries, with one dictionary per frame.
	  For each dictionary, the keys 'frame', 'x', 'y', 'view', 'labelled',
	  'present', 'ori' and 'phase' may be usedto access the relevant variables. 
	* image_dims -- 2-element list containing width and height of the video in
	  pixels
	* headup -- Boolean variable indicating the 'flip' of the video. True
	  indicates that the cross section is being viewed along the direction from
	  the fetal head to the fetal toes. False indicates the other direction.
	'''

	# Use above function to get the data
	table,image_dims,headup,radius = readHeartTrackFile(filename)

	# Create list of independent empty dictionaries
	dict_rows = [{} for _ in range(len(table))]

	# Loop and add data to the dictionaries
	for dict_row,table_row in zip(dict_rows,table):
		dict_row['frame'] = int(table_row[tk_frameCol])
		dict_row['y'] = int(table_row[tk_yposCol])
		dict_row['x'] = int(table_row[tk_xposCol])
		dict_row['view'] = int(table_row[tk_viewCol])
		dict_row['labelled'] = int(table_row[tk_labelledCol]) > 0
		dict_row['present'] = int(table_row[tk_presentCol])
		dict_row['ori'] = float(table_row[tk_oriCol])*(math.pi/180.0)
		dict_row['phase'] = float(table_row[tk_cardiacPhaseCol])

	return dict_rows,image_dims,headup,radius

# Function to read a single structure track from a trackfile
def readStructure(filename,structure):
	'''
	Read data for a single specified structure from a structure track (.stk)
	file.

	Arguments:
	* filename -- String containing file path and name for the .stk file to read
	  from
	* structure -- String containing name of structure to read

	Returns:
	* table -- numpy array containing data for the structure. Rows contain
	  frames, columns contain variables.
	  The columns may be indexed by the variables stk_frameCol, stk_labelledCol,
	  stk_presentCol, stk_yposCol, stk_xposCol, and stk_oriCol.
	  If the requested structure is not in the file, the return value will be
	  None.
	'''
	# Read in text in one monlithic block
	with open(filename,'r') as infile :
		text = infile.read()

	# Split into blocks based on empty lines
	textblocks = text.split("\n\n")

	# Loop through the blocks
	for block in textblocks[1:] :
		blocklines = block.split("\n")
		if not blocklines[0] :
			continue
		structname = blocklines[0].split()[1]

		if structname == substructure :
			table = np.zeros([len(blocklines)-1,6],dtype=int)

			# We have found the right block, now need to extract the data
			for i,dataline in enumerate(blocklines[1:]) :
				table[i,:] = np.fromstring(dataline,sep=" ")

			# We have found the right block, now need to extract the data
			for tableline,dataline in zip(blocklines[1:],table) :
				tableline = np.fromstring(dataline,sep=" ")

			return table

	return None


# Read a structure list and return a list of structures and other information
def readStructureList(filename) :
	names_list = []
	viewsPerStructure = []
	systole_only = []
	fourier_order = []

	# Open the file
	with open(filename,'r') as infile:
		for line in infile:
			names_list += [line.split()[0]]
			fourier_order += [int(line.split()[1])]
			systole_only += [line.split()[2] != '0']
			viewsPerStructure += [map(int,line.split()[3:])]

	# Largest view index used
	maxView = max(map(max,viewsPerStructure))

	# Find a list of the structures present in each view
	structuresPerView = [ [struct for struct in range(len(names_list)) if view in viewsPerStructure[struct]] for view in range(maxView+1)]

	return names_list, viewsPerStructure, structuresPerView, systole_only, fourier_order
