/*
 *  mxfcopy.cpp
 *  mxfapps
 *
 *  Created by Brendan Bolles on 1/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <iostream>
#include <exception>
#include <memory>
#include <map>
#include <vector>
#include <list>
#include <deque>

#include <assert.h>

#include <mxflib/mxflib.h>

#include <MoxMxf/StdIOStream.h>

using namespace std;
using namespace mxflib;



int main(int argc, char *argv[]) 
{
	if(argc == 3)
	{
		FileHandle input_fileH = NULL, output_fileH = NULL;
	
		try
		{
			input_fileH = MoxMxf::RegisterIOStream(new MoxMxf::StdIOStream(argv[1], MoxMxf::StdIOStream::ReadOnly));
			
			try
			{
				MXFInputFile input(input_fileH);
				
				const MXFInputFile::TrackMap &tracks = input.getTracks();
				
				Position startTimeCode = 0;
				Rational editRate(24, 1);
				
				EssenceList essence;
				
				UInt32 bodySID = 0;
				UInt32 indexSID = 0;
				
				for(MXFInputFile::TrackMap::const_iterator t = tracks.begin(); t != tracks.end(); ++t)
				{
					if(const MXFTimecodeTrack *timecode = dynamic_cast<const MXFTimecodeTrack *>(t->second))
					{
						startTimeCode = timecode->getStartTimecode();
						editRate = timecode->getEditRate();
					}
					else if(const MXFSourceTrack *source = dynamic_cast<const MXFSourceTrack *>(t->second))
					{
						essence[source->getNumber()] = source->getDescriptor();
					
						if(bodySID == 0)
						{
							bodySID = source->getBodySID();
							indexSID = source->getIndexSID();
						}
						else
						{
							assert(bodySID == source->getBodySID());
							assert(indexSID == source->getIndexSID());
						}
					}
				}
				
				if(bodySID == 0)
					throw my_exception("No bodySID");
				
				assert(indexSID != 0);
				
				
				output_fileH = MoxMxf::RegisterIOStream(new MoxMxf::StdIOStream(argv[2], MoxMxf::StdIOStream::ReadWrite));
				
				MXFOutputFile output(output_fileH, essence, editRate, startTimeCode);


				
				const Length duration = input.getDuration();

				
				for(int i=0; i < duration; i++)
				{
					MXFFramePtr frame = input.getFrame(i, bodySID, indexSID);
					
					if(frame)
					{
						cout << "frame " << i << " : ";
						
						const MXFFrame::FrameParts &parts = frame->getFrameParts();
						
						bool first_part = true;
						
						for(MXFFrame::FrameParts::const_iterator prt = parts.begin(); prt != parts.end(); ++prt)
						{
							const UInt32 frame_number = prt->first;
							MXFFramePartPtr part = prt->second;
							
							DataChunkPtr data = new DataChunk( part->getData() );
							
							assert(data->Size > 0);
							assert(data->Data != NULL);
							
							if(first_part)
							{
								output.PushEssence(frame_number, data, frame->getKeyOffset(), frame->getTemporalOffset(), frame->getFlags());
								first_part = false;
							}
							else
								output.PushEssence(frame_number, data);
							
							cout << data->Size << " ";
						}
						
						cout << endl;
					}
					else
						assert(false);
				}
			}
			catch(exception &e)
			{
				cerr << "ERROR: " << e.what() << endl;
			}
		}
		catch(...)
		{
			cerr << "Exception while opening file" << endl;
			return -1;
		}
		
		MoxMxf::DeleteIOStream(output_fileH);
		MoxMxf::DeleteIOStream(input_fileH);
	}
	else
	{
		cerr << "Usage: " << argv[0] << " <infile> <outfile>" << endl;
		return -1;
	}
	
	return 0;
}
