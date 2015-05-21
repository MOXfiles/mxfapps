/*
 *  mxfcopy.cpp
 *  mxfapps
 *
 *  Created by Brendan Bolles on 1/16/15.
 *  Copyright 2015 fnord. All rights reserved.
 *
 */

#include <iostream>

#include <assert.h>

#include <MoxMxf/InputFile.h>
#include <MoxMxf/OutputFile.h>

#include <MoxMxf/StdIOStream.h>

#include <MoxMxf/Exception.h>


using namespace MoxMxf;

int main(int argc, char *argv[]) 
{
	if(argc == 3)
	{
		try
		{
			StdIOStream infile(argv[1], StdIOStream::ReadOnly);
			
			try
			{
				InputFile input(infile);
				
				const InputFile::TrackMap &tracks = input.getTracks();
				
				Position startTimeCode = 0;
				Rational editRate(24, 1);
				
				OutputFile::EssenceList essence;
				
				SID bodySID = 0;
				SID indexSID = 0;
				
				for(InputFile::TrackMap::const_iterator t = tracks.begin(); t != tracks.end(); ++t)
				{
					if(const TimecodeTrack *timecode = dynamic_cast<const TimecodeTrack *>(t->second))
					{
						startTimeCode = timecode->getStartTimecode();
						editRate = timecode->getEditRate();
					}
					else if(const SourceTrack *source = dynamic_cast<const SourceTrack *>(t->second))
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
					throw InputExc("No bodySID");
				
				assert(indexSID != 0);
				
				
				StdIOStream outfile(argv[2], StdIOStream::ReadWrite);
				
				OutputFile output(outfile, essence, editRate, startTimeCode);


				
				const Length duration = input.getDuration();

				
				for(int i=0; i < duration; i++)
				{
					FramePtr frame = input.getFrame(i, bodySID, indexSID);
					
					if(frame)
					{
						std::cout << "frame " << i << " : ";
						
						const Frame::FrameParts &parts = frame->getFrameParts();
						
						bool first_part = true;
						
						for(Frame::FrameParts::const_iterator prt = parts.begin(); prt != parts.end(); ++prt)
						{
							const TrackNum track_number = prt->first;
							FramePartPtr part = prt->second;
							
							mxflib::DataChunkPtr data = new mxflib::DataChunk( part->getData() );
							
							assert(data->Size > 0);
							assert(data->Data != NULL);
							
							if(first_part)
							{
								output.PushEssence(track_number, data, frame->getKeyOffset(), frame->getTemporalOffset(), frame->getFlags());
								first_part = false;
							}
							else
								output.PushEssence(track_number, data);
							
							std::cout << data->Size << " ";
						}
						
						std::cout << std::endl;
					}
					else
						assert(false);
				}
			}
			catch(std::exception &e)
			{
				std::cerr << "ERROR: " << e.what() << std::endl;
			}
		}
		catch(...)
		{
			std::cerr << "Exception while opening file" << std::endl;
			return -1;
		}
	}
	else
	{
		std::cerr << "Usage: " << argv[0] << " <infile> <outfile>" << std::endl;
		return -1;
	}
	
	return 0;
}
