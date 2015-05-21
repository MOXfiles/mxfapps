/*
 *  mxfparse.cpp
 *  mxfapps
 *
 *  Created by Brendan Bolles on 12/3/14.
 *  Copyright 2014 fnord. All rights reserved.
 *
 */


#include <mxflib/mxflib.h>

#include <iostream>
#include <sstream>

#include <assert.h>

using namespace std;
using namespace mxflib;

#include "mxflib/dict.h"

#include <MoxMxf/StdIOStream.h>


static const bool DebugMode = true;

// Debug and error messages
/*
#include <stdarg.h>

#ifdef MXFLIB_DEBUG
//! Display a general debug message
void mxflib::debug(const char *Fmt, ...)
{
	if(!DebugMode) return;

	va_list args;

	va_start(args, Fmt);
	vprintf(Fmt, args);
	va_end(args);
}
#endif // MXFLIB_DEBUG

//! Display a warning message
void mxflib::warning(const char *Fmt, ...)
{
	va_list args;

	va_start(args, Fmt);
	printf("Warning: ");
	vprintf(Fmt, args);
	va_end(args);
}

//! Display an error message
void mxflib::error(const char *Fmt, ...)
{
	va_list args;

	va_start(args, Fmt);
	printf("ERROR: ");
	vprintf(Fmt, args);
	va_end(args);
}
*/

static string Ref2String(const TypeRef ref)
{
	return (ref == TypeRefUndefined ? "Undefined" :
			ref == TypeRefNone ? "None" :
			ref == TypeRefStrong ? "Strong" :
			ref == TypeRefWeak ? "Weak" :
			ref == TypeRefMeta ? "Meta" :
			ref == TypeRefDict ? "Dict" :
			ref == TypeRefGlobal ? "Global" :
			ref == TypeRefTarget ? "Target" :
			"Unknown");
}

static string Usage2String(const ClassUsage use)
{
	return (use == ClassUsageNULL ? "NULL" :
			use == ClassUsageOptional ? "Optional" :
			use == ClassUsageDecoderRequired ? "Decoder Required" :
			use == ClassUsageEncoderRequired ? "Encoder Required" :
			use == ClassUsageRequired ? "Required" :
			use == ClassUsageBestEffort ? "Best Effort" :
			use == ClassUsageToxic ? "Toxic" :
			use == ClassUsageDark ? "Dark" :
			"Unknown");
}

static string ContainerType2String(const MDContainerType typ)
{
	return (typ == NONE ? "None" :
			typ == SET ? "Set" :
			typ == PACK ? "Pack" :
			typ == BATCH ? "Batch" :
			typ == ARRAY ? "Array" :
			"Unknown");
}

static string Class2String(const MDTypeClass clas)
{
	return (clas == BASIC ? "Basic" :
			clas == INTERPRETATION ? "Interpretation" :
			clas == TYPEARRAY ? "TypeArray" :
			clas == COMPOUND ? "Compound" :
			clas == ENUM ? "Enum" :
			"Unknown");
}

static string ArrayClass2String(const MDArrayClass clas)
{
	return (clas == ARRAYIMPLICIT ? "Implicit" :
			clas == ARRAYEXPLICIT ? "Explicit" :
			clas == ARRAYSTRING ? "String" :
			"Unknown");
}

static string TrackType2String(const Track::TrackType type)
{
	return (type == Track::TrackTypeUndetermined ? "Undertermined" :
			type == Track::TrackTypeUnknown ? "Unknown" :
			type == Track::TrackTypeTimecode ? "Timecode" :
			type == Track::TrackTypeDescriptiveMetadata ? "DescriptiveMetadata" :
			type == Track::TrackTypePictureEssence ? "PictureEssence" :
			type == Track::TrackTypeSoundEssence ? "SoundEssence" :
			type == Track::TrackTypeDataEssence ? "DataEssence" :
			type == Track::TrackTypeAuxiliary ? "Auxiliary" :
			type == Track::TrackTypeParsedText ? "ParsedText" :
			"Dunno");
}
/*
static void PrintMDO_old(const MDObjectPtr mdata, const string prefix)
{
	cout << prefix << mdata->FullName() << " (" << mdata->GetType()->FullName() << ", " <<
		"Class: " << Class2String( mdata->GetClass() ) << ", " <<
		"Size: " << mdata->GetSize() << ", " <<
		"Ref: " << Ref2String( mdata->GetRefType() ) << ", " <<
		"Tag: " << mdata->GetTag() << ", " <<
		"Char: " << (mdata->IsCharacter() ? "True" : "False") << ", " <<
		"Endian: " << (mdata->GetEndian() ? "True" : "False") << ", " <<
		"Usage: " << Usage2String( mdata->GetUse() ) << ", " <<
		"Containter: " << ContainerType2String( mdata->GetContainerType() );
	
	cout << "): ";
	
	if(mdata->GetClass() != mdata->EffectiveClass())
	{
		cout << "EffectiveClass: " << Class2String( mdata->EffectiveClass() ) << " ";
	}
	
	if(mdata->GetClass() == ENUM)
	{
		cout << "(ENUM:";
	
		for(MDType::NamedValueList::const_iterator i = mdata->GetEnumValues().begin(); i != mdata->GetEnumValues().end(); ++i)
		{
			cout << " " << i->first << ":" << i->second->GetUInt();
		}
		
		cout << ") ";
	}
	
	if( mdata->IsAValue() )
	{
		const MDTypePtr val_type = mdata->GetValueType();
		
		const MDTypeClass val_class = val_type->EffectiveClass();
		const UL val_ul = *val_type->GetTypeUL();
		
		if(val_class == BASIC)
		{
			stringstream val;
			
			if(val_ul == Int8_UL || val_ul == Int16_UL || val_ul == Int32_UL)
			{
				val << mdata->GetInt();
			}
			else if(val_ul == UInt8_UL || val_ul == UInt16_UL || val_ul == UInt32_UL)
			{
				val << mdata->GetUInt();
			}
			else if(val_ul == Int64_UL)
			{
				val << mdata->GetInt64();
			}
			else if(val_ul == UInt64_UL)
			{
				val << mdata->GetUInt64();
			}
			else if(val_ul == LengthType_UL)
			{
				val << mdata->GetUInt64();
			}
			else
			{
				val << mdata->GetString();
			}
			
			cout << "(BASIC) (" << val_type->FullName() << "): " << val.str() << endl;
		}
		else if(val_class == INTERPRETATION)
		{
			cout << "(INTERPRETATION) (" << val_type->FullName() << "): " << mdata->GetString() << endl;
		}
		else if(val_class == TYPEARRAY)
		{
			cout << "(TYPEARRAY " << ArrayClass2String( mdata->GetArrayClass() ) << ") (" << val_type->FullName() << "): " << mdata->GetString() << endl;
		}
		else if(val_class == COMPOUND)
		{
			stringstream val;
			
			if(val_ul == Rational_UL)
			{
				// copied from mdtraits.cpp
				MDObjectPtr Numerator = mdata->Child("Numerator");
				MDObjectPtr Denominator = mdata->Child("Denominator");

				UInt32 Num = 0;
				UInt32 Den = 1;
				if(Numerator) Num = Numerator->GetUInt();
				if(Denominator) Den = Denominator->GetUInt();
			
				val << Num << "/" << Den;
			}
			else if(val_ul == Timestamp_UL)
			{
				MDObject *Year;
				MDObject *Month;
				MDObject *Day;
				MDObject *Hours;
				MDObject *Minutes;
				MDObject *Seconds;
				MDObject *msBy4;

				MDObject *Date = mdata->Child("Date");
				MDObject *Time = mdata->Child("Time");

				// AVMETA: Use Avid style nested structure if applicable
				if(Date && Time)
				{
					Year = Date->Child("Year");
					Month = Date->Child("Month");
					Day = Date->Child("Day");

					Hours = Time->Child("Hours");
					Minutes = Time->Child("Minutes");
					Seconds = Time->Child("Seconds");
					msBy4 = Time->Child("msBy4");
				}
				else
				{
					Year = mdata->Child("Year");
					Month = mdata->Child("Month");
					Day = mdata->Child("Day");
					Hours = mdata->Child("Hours");
					Minutes = mdata->Child("Minutes");
					Seconds = mdata->Child("Seconds");
					msBy4 = mdata->Child("msBy4");
				}

				UInt32 Y;
				UInt32 M;
				UInt32 D;
				UInt32 H;
				UInt32 Min;
				UInt32 S;
				UInt32 ms;

				if(Year) Y = Year->GetUInt(); else Y = 0;
				if(Month) M = Month->GetUInt(); else M = 0;
				if(Day) D = Day->GetUInt(); else D = 0;
				if(Hours) H = Hours->GetUInt(); else H = 0;
				if(Minutes) Min = Minutes->GetUInt(); else Min = 0;
				if(Seconds) S = Seconds->GetUInt(); else S = 0;
				if(msBy4) ms = msBy4->GetUInt() * 4; else ms = 0;
				
				val << Y << ":" << M << ":" << D << " " << H << ":" << Min << ":" << S << "." << ms;
			}
			else
			{
				val << mdata->GetString();
			}
		
			cout << "(COMPOUND) (" << val_type->FullName() << "): " << val.str() << endl;
		}
		else if(val_class == ENUM)
		{
			cout << "(ENUM) (" << val_type->FullName() << "): " << mdata->GetString() << endl;
		}
		else
		{
			cout << "(Unknown class " << val_class << ") (" << val_type->FullName() << "): " << mdata->GetString() << endl;
		}
	}
	else
	{
		cout << endl;
		
		if(mdata->GetClass() == TYPEARRAY && mdata->GetArrayClass() != ARRAYSTRING)
		{
			for(MDObject::const_iterator o_itr = mdata->begin(); o_itr != mdata->end(); ++o_itr)
			{
				PrintMDO_old(o_itr->second, prefix + "Iter (" + o_itr->first.GetString() + "): ");
			}
		}
	}



	const MDOTypeList &child_list = mdata->GetChildList();
	
	for(MDOTypeList::const_iterator t_itr = child_list.begin(); t_itr != child_list.end(); ++t_itr)
	{
		const MDOTypePtr m_type = *t_itr;
		
		const MDObjectPtr child = mdata->Child(m_type);
		
		if(child != NULL)
			PrintMDO_old(child, prefix + "Child: ");
		//else
		//	cout << prefix << "\t" << m_type->FullName() << ": NULL" << endl;
		
		//cout << prefix << "\t" << m_type->Name() << endl;
	}
	
	
	MDObjectParent ref = mdata->GetRef();
	
	if(ref.GetPtr() != NULL)
	{
		PrintMDO_old(ref, prefix + "Reference: ");
	}
}
*/

/*
class myHanlder : public GCReadHandler_Base
{
  public:
	myHanlder() {}
	virtual ~myHanlder() {}
	
	bool HandleData(GCReaderPtr Caller, KLVObjectPtr Object)
	{
		KLVObjectPtr &klv_element = Object;
									
		cout <<  klv_element->GetUL()->GetString() << " ";
		
		
		ULPtr theUL = klv_element->GetUL();
		
		if(theUL)
		{
			MDOTypePtr mdo_type = MDOType::Find(theUL);
			MDTypePtr md_type = MDType::Find(theUL);
			LabelPtr lab = Label::Find(theUL);
			
			if(mdo_type)
			{
				cout << "(" << mdo_type->Name() << ") ";
			}
			else if(md_type)
			{
				cout << "(" << md_type->Name() << ") ";
			}
			else if(lab)
			{
				cout << "(" << lab->GetName() << ") ";
			}
			else
				cout << "(unknown type) ";
		}
		else
			cout << "(no UL) ";
		
		
		cout << " Len: " << klv_element->GetLength() << " ";
		
		cout << endl;
		
		return true;
	}
};
*/

string DescribeUL(const UInt8 *ul)
{
	stringstream s;
	
	if(ul[0] == 0x06)
	{
		if(ul[1] == 0x0e)
		{
			if(ul[2] == 0x2b)
			{
				if(ul[3] == 0x34)
				{
					if(ul[4] == 0x01)
					{
						// see SMPTE 336M-2007 or Rec. ITU-R BT.1563-1
						
						s << "Element/Dictionary (see RP210) ";
						
						if(ul[5] == 0x01)
						{
							s << "Metadata Dictionary ";
							
							if(ul[6] == 0x01)
							{
								s << "(SMPTE 335M) ";
							}
						}
						else if(ul[5] == 0x02)
						{
							s << "Essence Dictionary ";
							
							if(ul[6] == 0x01)
							{
								// ul[7] is version
								if(ul[8] == 0x0d && ul[9] == 0x01 && ul[10] == 0x03)
								{
									// ul[11] is structure version
									
									s << "Generic Container ";
									
									if(ul[12] == 0x05)
									{
										s << "CP Picture (SMPTE 326M/331M) ";
									}
									else if(ul[12] == 0x06)
									{
										s << "CP Sound (SMPTE 326M/331M) ";
									}
									else if(ul[12] == 0x07)
									{
										s << "CP Data (SMPTE 326M/331M) ";
									}
									else if(ul[12] == 0x15)
									{
										s << "GC Picture ";
									}
									else if(ul[12] == 0x16)
									{
										s << "GC Sound ";
									}
									else if(ul[12] == 0x17)
									{
										s << "GC Data ";
									}
									else if(ul[12] == 0x18)
									{
										s << "GC Compound ";
									}
								}
							}
						}
						else if(ul[5] == 0x03)
						{
							s << "Control Dictionary ";
						}
						else if(ul[5] == 0x04)
						{
							s << "Type Dictionary ";
							
							if(ul[6] == 0x01)
							{
								s << "(Types Draft, CD2003) ";
							}
						}
					}
					else if(ul[4] == 0x02)
					{
						s << "KLV Set/Pack "; // aka "Group", SMPTE 395M
						
						if(ul[5] == 0x01)
						{
							s << "Universal set ";
						}
						else if(ul[5] & 0x0f == 0x02)
						{
							s << "Global set ";
							
							if(ul[5] == 0x02)
							{
								s << "any length ";
							}
							else if(ul[5] == 0x22)
							{
								s << "length up to 255 ";
							}
							else if(ul[5] == 0x42)
							{
								s << "length up to 2^8 ";
							}
							else if(ul[5] == 0x62)
							{
								s << "length up to 2^32 ";
							}
						}
						else if(ul[5] & 0x0f == 0x03 || ul[5] & 0x0f == 0x0b)
						{
							s << "Local set ";
							
							if(ul[5] & 0xf0 == 0x00 || ul[5] & 0xf0 == 0x10)
							{
								s << "any length ";
							}
							else if(ul[5] & 0xf0 == 0x20 || ul[5] & 0xf0 == 0x30)
							{
								s << "1 byte length ";
							}
							else if(ul[5] & 0xf0 == 0x40 || ul[5] & 0xf0 == 0x50)
							{
								s << "2 bytes length ";
							}
							else if(ul[5] & 0xf0 == 0x60 || ul[5] & 0xf0 == 0x70)
							{
								s << "4 bytes length ";
							}
							
							if(ul[5] & 0x0f == 0x03)
							{
								if(ul[5] & 0xf0 == 0x00 || ul[5] & 0xf0 == 0x20 || ul[5] & 0xf0 == 0x40 || ul[5] & 0xf0 == 0x60)
								{
									s << "1 byte tag ";
								}
								else if(ul[5] & 0xf0 == 0x10 || ul[5] & 0xf0 == 0x30 || ul[5] & 0xf0 == 0x50 || ul[5] & 0xf0 == 0x70)
								{
									s << "2 bytes tag ";
								}
							}
							else if(ul[5] & 0x0f == 0x0b)
							{
								if(ul[5] & 0xf0 == 0x00 || ul[5] & 0xf0 == 0x20 || ul[5] & 0xf0 == 0x40 || ul[5] & 0xf0 == 0x60)
								{
									s << "any length tag ";
								}
								else if(ul[5] & 0xf0 == 0x10 || ul[5] & 0xf0 == 0x30 || ul[5] & 0xf0 == 0x50 || ul[5] & 0xf0 == 0x70)
								{
									s << "4 bytes tag ";
								}
							}
						}
						else if(ul[5] & 0x0f == 0x04)
						{
							s << "Variable length pack ";
							
							if(ul[5] & 0xf0 == 0x00)
							{
								s << "any length ";
							}
							else if(ul[5] & 0xf0 == 0x20)
							{
								s << "1 byte length ";
							}
							else if(ul[5] & 0xf0 == 0x40)
							{
								s << "2 bytes length ";
							}
							else if(ul[5] & 0xf0 == 0x60)
							{
								s << "4 bytes length ";
							}
						}
						else if(ul[5] == 0x05)
						{
							s << "Fixed length "; // aka "Defined-Length Pack"
						}
						else if(ul[5] == 0x06)
						{
							s << "Reserved ";
						}
						else if(ul[5] == 0x53)
						{
							s << "Local set (2-byte tag, 2-byte length)";
							// see SMPTE 377M page 44
						}
						
						
						if(ul[6] == 0x01)
						{
							// see SMPTE 336M for ul[5] registry
							// ul[7] is version
							
							if(ul[8] == 0x0d && ul[9] == 0x01 && ul[10] == 0x03 && ul[11] == 0x01)
							{
								s << "system item ";
								
								if(ul[12] == 0x04 || ul[12] == 0x14)
								{
									s << "CP compatible ";
									
									if(ul[12] == 0x04)
									{
										s << "(SMPTE 326M) ";
									}
								}
							}
							else
							{
								s << "See sets and packs registry ";
							}
							
						}
					}
					else if(ul[4] == 0x03)
					{
						s << "Wrapper/Container ";
						
						if(ul[5] == 0x01)
						{
							s << "Simple Wrapper ";
						}
						else if(ul[5] == 0x02)
						{
							s << "Complex wrapper ";
						}
					}
					else if(ul[4] == 0x04)
					{
						s << "Label (see RP224) ";
						
						if(ul[5] == 0x01)
						{
							s << "(SMPTE 400M) ";
							
							if(ul[6] == 0x01 && ul[8] == 0x0d && ul[9] == 0x01 && ul[10] == 0x03)
							{
								if(ul[12] == 0x02)
								{
									s << "Generic container ";
								}
								else if(ul[12] == 0x01)
								{
									s << "*forbidden ";
								}
							}
						}
					}
					else if(ul[4] == 0x05)
					{
						s << "Registered Private Information ";
					}
					else
						s << "Unknown designation";
				}
				else
					s << "NOT SMPTE organization, ID " << (int)ul[3];
			}
			else
			{
				const int ituiso = ul[2] / 40;
				const int subcat = ul[2] % 40;
			
				s << "NOT SMPTE UL: ";
				
				if(ituiso == 0)
				{
					s << "ITU ";
					
					switch(subcat)
					{
						case 0:	s << "recommendation ";	break;
						case 1:	s << "question ";	break;
						case 2:	s << "administration ";	break;
						case 3:	s << "network operator ";	break;
						default: s << "ERROR: Bad ITU Category"; break;
					}
				}
				else if(ituiso == 1)
				{
					s << "ISO ";
					
					switch(subcat)
					{
						case 0:	s << "standard ";	break;
						case 1:	s << "ERROR: unused subcat ";	break;
						case 2:	s << "member body ";	break;
						case 3:	s << "organization " << (int)ul[3];	break;
						default: s << "ERROR: Bad ISO Category"; break;
					}
				}
				else if(ituiso == 2)
				{
					s << "Joint ISO/ITU";
				}
				else
					s << "BAD ISO/ITU identifier";
			}
		}
		else if(ul[1] == 0x0a && ul[2] == 0x2b && ul[3] == 0x34 &&
				ul[4] == 0x01 && ul[5] == 0x01 && ul[6] == 0x01 && ul[7] == 0x01)
		{
			s << "NOT SMPTE UL: maybe Element (see RP210)"; // could break this down more
		}
		else
			s << "NOT SMPTE UL: size is " << (int)ul[1];
	}
	else if(ul[0] == 0x26)
	{
		s << "Constructed encoding, size " << (int)ul[1];
	}
	else
		s << "NOT UL: bad identifier";
	
	return s.str();
}

string DescribeUL(const ULPtr ul)
{
	return DescribeUL( ul->GetValue() );
}


static void PrintMDO(const MDObjectPtr mdata, const string prefix)
{
	const string current_loc = prefix + mdata->Name();

	cout << current_loc;
	
	if( mdata->IsAValue() )
	{
		if( mdata->IsDValue() )
			cout << " DVALUE";
	
		if(mdata->GetValueType() == MDType::Find(UL_UL))
		{
			UL theUL(mdata->GetData().Data);
		
			cout << " <UL> " << theUL.GetString() << " ";
			
			MDOTypePtr mdo_type = MDOType::Find(theUL);
			MDTypePtr md_type = MDType::Find(theUL);
			LabelPtr lab = Label::Find(theUL);
			
			if(mdo_type)
			{
				cout << mdo_type->GetDetail() << " ";
			}
			else if(md_type)
			{
				cout << md_type->GetDetail() << " ";
			}
			else if(lab)
			{
				cout << lab->GetDetail() << " ";
			}
			else
				cout << "(Not in Dictionary) ";

			cout << "-- " << DescribeUL(theUL.GetValue()) << endl;
		}
		else if(mdata->GetValueType() == MDType::Find(AUID_UL))
		{
			UL theUL(mdata->GetData().Data);
		
			cout << " <AUIDDDD> " << theUL.GetString() << " ";
			
			MDOTypePtr mdo_type = MDOType::Find(theUL);
			MDTypePtr md_type = MDType::Find(theUL);
			LabelPtr lab = Label::Find(theUL);
			
			if(mdo_type)
			{
				cout << mdo_type->GetDetail() << " ";
			}
			else if(md_type)
			{
				cout << md_type->GetDetail() << " ";
			}
			else if(lab)
			{
				cout << lab->GetDetail() << " ";
			}
			else
				cout << "(Not in Dictionary) ";

			cout << "-- " << DescribeUL(theUL.GetValue()) << endl;
		}
		else if(mdata->GetValueType() == MDType::Find(UUID_UL))
		{
			UUID pUL(mdata->GetData().Data);
		
			cout << " <UUID> " << pUL.GetString() << endl;
		}
		else if(mdata->GetValueType() == MDType::Find(UMID_UL))
		{
			UMID pID(mdata->GetData().Data);
			
			cout << " <UMID> " << mdata->GetString() << endl;
		}
		else if(mdata->GetValueType() == MDType::Find(PackageID_UL))
		{
			UMID pID(mdata->GetData().Data);
			
			cout << " <PackageID> " << mdata->GetString() << endl;
		}
		else if(mdata->GetValueType() == MDType::Find(UUID_UL))
		{
			UUID uID(mdata->GetData().Data);
			
			cout << " <UUID> " << mdata->GetString() << endl;
		}
		else if(mdata->GetValueType() == MDType::Find(DictReferenceContainerDefinition_UL))
		{
			UL theUL(mdata->GetData().Data);

			cout << " <DictReferenceContainerDefinition> ";
			
			MDOTypePtr mdo_type = MDOType::Find(theUL);
			MDTypePtr md_type = MDType::Find(theUL);
			LabelPtr lab = Label::Find(theUL);
			
			if(mdo_type)
			{
				cout << mdo_type->GetDetail() << " ";
			}
			else if(md_type)
			{
				cout << md_type->GetDetail() << " ";
			}
			else if(lab)
			{
				cout << lab->GetDetail() << " ";
			}
			
			cout << "-- " << DescribeUL(theUL.GetValue()) << endl;
		}
		else if(mdata->GetValueType() == MDType::Find(Rational_UL))
		{
			MDObjectPtr Numerator = mdata->Child("Numerator");
			MDObjectPtr Denominator = mdata->Child("Denominator");
			
			//ULPtr num_txt = mdata->Child("Numerator")->GetUL();
			//ULPtr den_txt = mdata->Child("Numerator")->GetUL();
			
			//assert(*num_txt == Numerator_UL);
			//assert(*den_txt == Denominator_UL);
		
			Rational rat(Numerator->GetInt(), Denominator->GetInt());
			
			cout << " <Rational> " << rat.Numerator << " / " << rat.Denominator << endl;
		}
		else
		{
			cout << ": (" << Class2String( mdata->GetClass() ) << ", " <<
				ContainerType2String( mdata->GetContainerType() ) << ") ";
		
			if(mdata->GetClass() == TYPEARRAY)
			{
				if(mdata->GetArrayClass() == ARRAYSTRING)
				{
					cout << "ARRAYSTRING: " << mdata->GetString() << endl;
				}
				else if(mdata->GetArrayClass() == ARRAYIMPLICIT)
				{
					cout << "ARRAYIMPLICIT: " << mdata->GetString() << endl;
				}
				else
				{
					assert(mdata->GetArrayClass() == ARRAYEXPLICIT);
					assert(mdata->GetData().Size == 0);
											
					cout << " (Array) " << endl;
					
					int i = 0;
					
					for(MDObject::const_iterator o_itr = mdata->begin(); o_itr != mdata->end(); ++o_itr)
					{
						stringstream loc;
						
						loc << current_loc << "[" << i++ << "]: ";
						
						PrintMDO(o_itr->second, loc.str()); 
					}
					
					if(i == 0)
						cout << current_loc << " {empty array}" << endl;
				}
			}
			else
			{
				cout << "(" << mdata->GetValueType()->FullName() << ") " << mdata->GetString() << endl;
			}
		}
	}
	else
	{
		if(mdata->GetClass() == TYPEARRAY)
		{
			if(mdata->GetArrayClass() == ARRAYSTRING)
			{
				assert(false); // only execting when IsAValue()
				cout << "ARRAYSTRING: " << mdata->GetString() << endl;
			}
			else if(mdata->GetArrayClass() == ARRAYIMPLICIT)
			{
				assert(false); // only execting when IsAValue()
				cout << "ARRAYIMPLICIT: data size: " << mdata->GetData().Size << endl;
			}
			else
			{
				assert(mdata->GetArrayClass() == ARRAYEXPLICIT);
				assert(mdata->GetData().Size == 0);
				
				cout << " (Array)" << endl;
				
				int i = 0;
				
				for(MDObject::const_iterator o_itr = mdata->begin(); o_itr != mdata->end(); ++o_itr)
				{
					stringstream loc;
					
					loc << current_loc << "[" << i++ << "]: ";
					
					PrintMDO(o_itr->second, loc.str()); 
				}
				
				if(i == 0)
					cout << current_loc << " {empty array}" << endl;
			}
		}
		else
		{
			cout << " (" << Class2String( mdata->GetClass() ) << ", " <<
				ContainerType2String( mdata->GetContainerType() ) << ")" << endl;
		}
	}
	
	
	const MDOTypeList &child_list = mdata->GetChildList();
	
	for(MDOTypeList::const_iterator t_itr = child_list.begin(); t_itr != child_list.end(); ++t_itr)
	{
		const MDOTypePtr m_type = *t_itr;
		
		const MDObjectPtr child = mdata->Child(m_type);
		
		if(child != NULL)
			PrintMDO(child, current_loc + "/");
	}
	
	
	
	MDObjectParent ref = mdata->GetRef();
	
	if(ref)
	{
		if(mdata->GetRefType() == TypeRefStrong)
		{
			PrintMDO(ref, current_loc + "->");
		}
		else
		{
			assert(mdata->GetRefType() == TypeRefWeak);
		
			cout << current_loc << "~>" << ref->Name() << endl;
		}
	}
	else
	{
		if(mdata->GetRefType() == TypeRefTarget)
		{
			const MDObjectParent &par = mdata->GetParent();
			
			assert(par);
		}
		else if(mdata->GetRefType() == TypeRefDict)
		{
			LabelPtr label = Label::Find(mdata->GetData().Data);
			
			if(label)
			{
				string txt = label->GetDetail();
				
				assert(txt == mdata->GetString());
			}
			else
				assert(false);
		}
		else
		{
			TypeRef typ = mdata->GetRefType();
		
			if(mdata->GetRefType() != TypeRefNone)
			{
				cout << "** Weird, no ref but ref type is " << Ref2String(mdata->GetRefType()) << endl;
			}
		}
	}
}


int main(int argc, char *argv[]) 
{
	try
	{
		if(argc == 2)
		{
			LoadDictionary(DictData);
			
			//06.0E.2B.34.01.01.01.05.03.01.02.20.01.00.00.00
			//const UInt8 XML_UL_Data[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x05, 0x03, 0x01, 0x02, 0x20, 0x01, 0x00, 0x00, 0x00 };
			//const UL XML_UL(XML_UL_Data);

			try
			{
				FileHandle file_handle = MoxMxf::RegisterIOStream(new MoxMxf::StdIOStream(argv[1], MoxMxf::StdIOStream::ReadOnly));
			
				MXFFilePtr file = new MXFFile;
				
				if( file->OpenFromHandle(file_handle) )
				{
					cout << "File Name: " << file->Name << endl;
					cout << "RunIn Size: " << file->RunIn.Size << endl;
					
				
					PartitionPtr partition = file->ReadMasterPartition();
					
					file->GetRIP();
					
					//file->Seek(0);
					
					//PartitionPtr partition = file->ReadPartition();
					
					if(partition)
					{
						if( partition->ReadMetadata() )
						{
							cout << "\tMaster Partition" << " (" << partition->GetType()->FullName() << ", " <<
								(partition->IsComplete() ? "" : "NOT ") << "Complete, " <<
								(partition->IsClosed() ? "" : "NOT ") << "Closed)" << endl;
							
							for(MDObjectList::iterator p_it = partition->TopLevelMetadata.begin(); p_it != partition->TopLevelMetadata.end(); ++p_it)
							{
								PrintMDO(*p_it, "\t\t");
							}
							
							
							cout << "==================================" << endl;
							
							
							MetadataPtr metadata = partition->ParseMetadata();
						
							if(metadata)
							{
								PackagePtr primary_package = metadata->GetPrimaryPackage();
							
								for(PackageList::iterator p = metadata->Packages.begin(); p != metadata->Packages.end(); ++p)
								{
									PackagePtr pack = *p;
								
									if(*pack->GetUL() == MaterialPackage_UL)
									{
										cout << "Material Package" << endl;
									}
									else if(*pack->GetUL() == SourcePackage_UL)
									{
										cout << "Source Package" << endl;
									}
									
									cout << "Package UID: " << pack->GetString(PackageUID_UL) << endl;
									cout << "Package Name: " << pack->GetString(PackageName_UL) << endl;
									cout << "Creation Date: " << pack->GetString(PackageCreationDate_UL) << endl;
									cout << "Modification Date: " << pack->GetString(PackageModifiedDate_UL) << endl;
									
									
									if(*pack->GetUL() == SourcePackage_UL)
									{
										MDObjectPtr descriptor_ref = pack->Child(Descriptor_UL);
										
										if(descriptor_ref)
										{
											MDObjectPtr descriptor = descriptor_ref->GetRef();
											
											if(*descriptor->GetUL() == MultipleDescriptor_UL)
											{
												cout << "\t" << "MULTI-DESCRIPTOR" << endl;
											
												MDObjectPtr descriptor_ref_array = descriptor->Child(FileDescriptors_UL);
												
												if(descriptor_ref_array)
												{
													for(MDObject::const_iterator o_itr = descriptor_ref_array->begin(); o_itr != descriptor_ref_array->end(); ++o_itr)
													{
														MDObjectPtr descriptor_i = o_itr->second->GetRef();
														
														cout << "\t\t" << "DESCRIPTOR" << endl;
														cout << "\t\t" << "Name: " << descriptor_i->Name() << endl;
														cout << "\t\t" << "Linked Track ID: " << descriptor_i->GetInt(LinkedTrackID_UL) << endl;
													}
												}
											}
											else
											{
												cout << "\t" << "DESCRIPTOR" << endl;
												cout << "\t" << "Name: " << descriptor->Name() << endl;
												cout << "\t" << "Linked Track ID: " << descriptor->GetInt(LinkedTrackID_UL) << endl;
											}
											
											//CDCIEssenceDescriptor_UL
										}
									}
									
									
									for(TrackList::iterator t = pack->Tracks.begin(); t != pack->Tracks.end(); ++t)
									{
										TrackPtr track = *t;
										
										cout << "\t" << "TRACK" << endl;
										cout << "\t" << "Type: " << TrackType2String(track->GetTrackType()) << endl;
										//cout << "\t" << "Word: " << track->GetTrackWord() << endl;
										cout << "\t" << "Name: " << track->GetString(TrackName_UL) << endl;
										cout << "\t" << "ID: " << track->GetInt(TrackID_UL) << endl;
										cout << "\t" << "Number: " << track->GetInt(TrackNumber_UL) << endl;
										cout << "\t" << "Origin: " << track->GetInt64(Origin_UL) << endl;
										cout << "\t" << "Framerate: " << track->Child(EditRate_UL)->GetInt("Numerator") << " / " <<
											track->Child(EditRate_UL)->GetInt("Denominator") << endl;
										
										for(ComponentList::iterator c = track->Components.begin(); c != track->Components.end(); ++c)
										{
											ComponentPtr comp = *c;
											
											cout << "\t\t" << "COMPONENT" << endl;
											cout << "\t\t" << comp->Name() << endl;
											
											TimecodeComponentPtr timecode_component = TimecodeComponent::GetTimecodeComponent(comp->Object);
											SourceClipPtr source_clip = SourceClip::GetSourceClip(comp->Object);
											DMSegmentPtr dmsegment = DMSegment::GetDMSegment(comp->Object);
											
											if(timecode_component)
											{
												assert(*comp->GetUL() == TimecodeComponent_UL);
												cout << "\t\t\t" << "TIMECODE COMPONENT" << endl;
												cout << "\t\t\t" << "Framerate: " << timecode_component->GetUInt(RoundedTimecodeBase_UL) << endl;
												cout << "\t\t\t" << "Start: " << timecode_component->GetInt64(StartTimecode_UL) << endl;
												cout << "\t\t\t" << "Duration: " << timecode_component->GetInt64(ComponentLength_UL) << endl;
												cout << "\t\t\t" << "Drop Frame: " << (timecode_component->GetInt(DropFrame_UL) ? "True" : "False") << endl;
												cout << "\t\t\t" << "Data Definition: " << timecode_component->Child(ComponentDataDefinition_UL)->GetString() << endl;
											}
											else if(source_clip)
											{
												assert(*comp->GetUL() == SourceClip_UL);
												cout << "\t\t\t" << "SOURCE CLIP" << endl;
												cout << "\t\t\t" << "Duration: " << source_clip->GetInt64(ComponentLength_UL) << endl;
												cout << "\t\t\t" << "Source Track ID: " << source_clip->GetInt(SourceTrackID_UL) << endl;
												cout << "\t\t\t" << "Source Package ID: " << source_clip->Child(SourcePackageID_UL)->GetString() << endl;
												cout << "\t\t\t" << "Data Definition: " << source_clip->Child(ComponentDataDefinition_UL)->GetString() << endl;
											}
											else if(dmsegment)
											{
												assert(*comp->GetUL() == DMSegment_UL);
												cout << "\t\t\t" << "DESCRIPTIVE METADATA SEGMENT" << endl;
												cout << "\t\t\t" << "Duration: " << dmsegment->GetInt64(ComponentLength_UL) << endl;
												cout << "\t\t\t" << "Data Definition: " << dmsegment->Child(ComponentDataDefinition_UL)->GetString() << endl;
											}
										}
									}
								}
								
								
								MDObjectPtr content_storage_ref = metadata[ContentStorageObject_UL];
								
								if(content_storage_ref && content_storage_ref->GetRef())
								{
									MDObjectPtr content_storage = content_storage_ref->GetRef();
								
									MDObjectPtr essence_data_objects = content_storage[EssenceDataObjects_UL];
									
									int i = 0;
									
									for(MDObject::const_iterator o_itr = essence_data_objects->begin(); o_itr != essence_data_objects->end(); ++o_itr)
									{
										MDObjectPtr essence_data_ref = o_itr->second;
										
										MDObjectPtr essence_data = essence_data_ref->GetRef();
										
										//PrintMDO(essence_data, "==");
										cout << "Essence Data[" << i++ << "]" << endl;
										cout << "Linked Package UID: " << essence_data->GetString(LinkedPackageUID_UL) << endl;
										cout << "BodySID: " << essence_data->GetInt(BodySID_UL) << endl;
										
										if(essence_data->Child(IndexSID_UL))
											cout << "IndexSID: " << essence_data->GetInt(IndexSID_UL) << endl;
									}
								}
							}
						}
						else
							cout << "\tNo metadata in partition" << endl;
					}
					else
						cout << "\tFailed to read partition" << endl;
					
					
					cout << "==================================" << endl;
					
					
					if( file->GetRIP() )  // Random Index Pack
					{
						IndexTablePtr Table = new IndexTable;
					
						unsigned int PartitionNumber = 0;
						
						for(RIP::iterator it = file->FileRIP.begin(); it != file->FileRIP.end(); ++it)
						{
							cout << "Partition " << PartitionNumber++ << " (" << it->first << ")" << endl;
							
							PartitionInfoPtr p_info = it->second;
							
							file->Seek(p_info->ByteOffset);
							
							PartitionPtr partition = file->ReadPartition();
							
							if(partition)
							{
								//cout << "\t" << partition->GetUL()->GetString() << " " << DescribeUL(partition->GetUL()) << endl;
								
								if(partition->IsA(OpenHeader_UL))
									cout << "\t" << "Open Header" << endl;
								
								if(partition->IsA(OpenCompleteHeader_UL))
									cout << "\t" << "Open Complete Header" << endl;
									
								if(partition->IsA(ClosedHeader_UL))
									cout << "\t" << "Closed Header" << endl;
								
								if(partition->IsA(ClosedCompleteHeader_UL))
									cout << "\t" << "Closed Complete Header" << endl;
									
								if(partition->IsA(OpenBodyPartition_UL))
									cout << "\t" << "Open Body" << endl;
								
								if(partition->IsA(OpenCompleteBodyPartition_UL))
									cout << "\t" << "Open Complete Body" << endl;
									
								if(partition->IsA(ClosedBodyPartition_UL))
									cout << "\t" << "Closed Body" << endl;
								
								if(partition->IsA(ClosedCompleteBodyPartition_UL))
									cout << "\t" << "Closed Complete Body" << endl;
									
								if(partition->IsA(Footer_UL))
									cout << "\t" << "Footer (non-partition)" << endl;
									
								if(partition->IsA(FooterPartition_UL))
									cout << "\t" << "Footer" << endl;
								
								if(partition->IsA(CompleteFooter_UL))
									cout << "\t" << "Complete Footer" << endl;
									
								cout << "\t" << "Major Version: " << partition->GetUInt(MajorVersion_UL) << endl;
								cout << "\t" << "Minor Version: " << partition->GetUInt(MinorVersion_UL) << endl;
								cout << "\t" << "KAG Size: " << partition->GetUInt(KAGSize_UL) << endl;
								cout << "\t" << "BodySID: " << partition->GetUInt(BodySID_UL) << endl;
								cout << "\t" << "IndexSID: " << partition->GetUInt(IndexSID_UL) << endl;
								cout << "\t" << "Operational Pattern: " << partition->GetString(OperationalPattern_UL) << endl;
								
								cout << "\t" << "Essence Containers: ";
								
								PrintMDO(partition->Child(EssenceContainers_UL), "\t\t");
								
								
								
								partition->StartElements();
								
								KLVObjectPtr klv_element;
								int i = 0;
								
								Position essence_start = 0;
								
								while( (klv_element = partition->NextElement() ) )
								{
									// Generic container keys start 060e2b34.0102.01XX.0d0103 (XX is version number)
									// see GCEssenceKeyNorm in essence.cpp
								
									if(essence_start == 0)
										essence_start = klv_element->GetLocation();
									
									const GCElementKind kind = klv_element->GetGCElementKind();
									
									cout << i++ << " " << klv_element->GetUL()->GetString() << " ";
									
									
									ULPtr theUL = klv_element->GetUL();
									
									if(theUL)
									{
										MDOTypePtr mdo_type = MDOType::Find(theUL);
										MDTypePtr md_type = MDType::Find(theUL);
										LabelPtr lab = Label::Find(theUL);
										
										if(mdo_type)
										{
											cout << "(MDOType: " << mdo_type->Name() << ") ";
										}
										else if(md_type)
										{
											cout << "(MDType: " << md_type->Name() << ") ";
										}
										else if(lab)
										{
											cout << "(Label: " << lab->GetName() << ") ";
										}
										else
											cout << DescribeUL(theUL);
									}
									else
										cout << "(no UL) ";
									
									
									cout << " Len: " << klv_element->GetLength() << " ";
									
									
									if(kind.IsValid)
									{
										assert( !klv_element->IsGCSystemItem() );
									
										cout << " Item: " << (int)kind.Item << " " <<
											" Count: " << (int)kind.Count << " " <<
											" ElementType: " << (int)kind.ElementType << " " << // SMPTE 331M
											" Number: " << (int)kind.Number << " " <<
											" Track Number: " << klv_element->GetGCTrackNumber() << " (0x" << Int64toHexString(klv_element->GetGCTrackNumber()) << ") ";
										
										
										size_t data_size = klv_element->ReadData();
										
										DataChunk &data = klv_element->GetData();
										
										assert(klv_element->GetLength() == data_size);
									}
									else if( klv_element->IsGCSystemItem() )
									{
										cout << "(System Element)";
									}
									else
									{
										cout << "(not GC element)";
									}
									
									cout << " Location: " << Int64toHexString(klv_element->GetLocation(), 8);
									cout << " Delta: " << Int64toHexString(klv_element->GetLocation() - essence_start, 8) << endl;
								}
								
								
								cout << endl;
								
								
								MDObjectListPtr segments = partition->ReadIndex();
								
								if(segments && (partition->GetInt64(IndexByteCount_UL) > 0))
								{
									cout << "INDEX" << endl;
								
									for(MDObjectList::iterator it = segments->begin(); it != segments->end(); ++it)
									{
										Table->AddSegment(*it);
										
										cout << (*it)->Name() << endl;
									}
									
									cout << "IndexDuration: " << Table->IndexDuration << endl;
									cout << "Duration: " << Table->GetDuration() << endl;
									cout << "IndexSID: " << Table->IndexSID << endl;
									cout << "BodySID: " << Table->BodySID << endl;
									cout << "EditRate: " << Table->EditRate.Numerator << " / " << Table->EditRate.Denominator << endl;
									cout << "EditUnitByteCount: " << Table->EditUnitByteCount << endl;
									cout << "BaseDeltaCount: " << Table->BaseDeltaCount << endl;
									
									cout << "BASE DELTAS" << endl;
									
									for(int i=0; i < Table->BaseDeltaCount; i++)
									{
										const DeltaEntry *delt = &Table->BaseDeltaArray[i];
										
										UInt32 *element_delta = (UInt32 *)delt->ElementDelta;
										
										cout << i << ": Index: " << (Int32)delt->PosTableIndex << " Slice: " << (Int32)delt->Slice << " ElementDelta: " << Int64toHexString(*element_delta, 8) << endl;
									}

									/*
									for(IndexSegmentMap::iterator s = Table->SegmentMap.begin(); s != Table->SegmentMap.end(); ++s)
									{
										const Position &pos = s->first;
										const IndexSegmentPtr &seg = s->second;
										
										cout << "SEGMENT" << endl;
										
										cout << "\t" << "Pos: " << pos << "  StartPos: " << seg->StartPosition << endl;
										
										cout << "\t" << "DELTAS" << endl;
										
										for(int i=0; i < seg->DeltaCount; i++)
										{
											const DeltaEntry *delt = &seg->DeltaArray[i];
											
											UInt32 *element_delta = (UInt32 *)delt->ElementDelta;
											
											cout << "\t" << i << ": Index: " << (Int32)delt->PosTableIndex << " Slice: " << (Int32)delt->Slice << " ElementDelta: " << Int64toHexString(*element_delta, 8) << endl;
										}
										
										
										cout << "\t" << "ENTRIES" << endl;
										
										for(int i=0; i < seg->EntryCount; i++)
										{
											const IndexEntry *entries = (const IndexEntry *)seg->IndexEntryArray.Data;
											
											const IndexEntry &entry = entries[i];
											
											cout << "\t" << i << ": Time: " << Int64toHexString(entry.TemporalOffset, 8) << " AnchorOffset: " << Int64toHexString(entry.AnchorOffset, 8) <<
												" Flags: " << entry.Flags << " StreamOffset: " << Int64toHexString(entry.StreamOffset, 8) << endl;
										}
									}*/
									
									
									cout << "POSITIONS" << endl;
									
									for(int i=0; i < Table->GetDuration(); i++)
									{
										IndexPosPtr index_pos = Table->Lookup(i);
										
										if(index_pos)
										{
											assert(i == index_pos->ThisPos);
										
											cout << i <<
												": Location: " << Int64toHexString(index_pos->Location, 8) << 
												": PosOffset: " << index_pos->PosOffset.Numerator << " / " << index_pos->PosOffset.Denominator <<
												": Exact: " << (index_pos->Exact ? "True" : "False") <<
												": OtherPos: " << (index_pos->OtherPos ? "True" : "False") <<
												": Offset: " << (index_pos->Offset ? "True" : "False") <<
												": KeyFrameOffset: " << (Int32)index_pos->KeyFrameOffset <<
												": TemporalOffset: " << (Int32)index_pos->TemporalOffset <<
												": KeyLocation: " << Int64toHexString(index_pos->KeyLocation, 8);
												
											cout << ": Flags: 0x" << Int64toHexString(index_pos->Flags, 2) << " ( " << // 377M page 58
												((index_pos->Flags & (1L << 0)) ? "Bit0 " : "") <<
												((index_pos->Flags & (1L << 1)) ? "Bit1 " : "") <<
												((index_pos->Flags & (1L << 2)) ? "Bit2 " : "") <<
												((index_pos->Flags & (1L << 3)) ? "Bit3 " : "") <<
												((index_pos->Flags & (1L << 4)) ? "BackwardPrediction " : "") <<
												((index_pos->Flags & (1L << 5)) ? "ForwardPrediction " : "") <<
												((index_pos->Flags & (1L << 6)) ? "SequenceHeader " : "") <<
												((index_pos->Flags & (1L << 7)) ? "RandomAccess " : "") <<
												")" << endl;
												
										}
									}
								}
								else
									cout << "NO INDEX" << endl;
							}
							
							cout << endl;
						}
					}
					
					
					/*
					cout << "==================================" << endl;
					
					
					BodyReaderPtr reader = new BodyReader(file);
					
					if(reader)
					{
						//reader->Seek(0);
					
						const UInt32 BodySID = 2;
						
						// GC = Generic Container
						
						GCReadHandlerPtr handler = new myHanlder;
						
						bool made = reader->MakeGCReader(BodySID, handler);
						
						while( reader->ReadFromFile() )
						{
						
						}
					}
					*/
					
					
					/*if( file->GetRIP() )  // Random Index Pack
					{
						unsigned int PartitionNumber = 0;
						for(RIP::iterator it = file->FileRIP.begin(); it != file->FileRIP.end(); ++it)
						{
							PartitionNumber++;
							cout << "Partition " << PartitionNumber << " (" << it->first << ")" << endl;
							
							PartitionInfoPtr p_info = it->second;
							
							file->Seek(p_info->ByteOffset);
							PartitionPtr partition = file->ReadPartition();
							
							if(partition)
							{
								if( partition->ReadMetadata() )
								{
									cout << "\tPartition " << PartitionNumber << " (" << partition->GetType()->FullName() << ", " <<
										(partition->IsComplete() ? "" : "NOT ") << "Complete, " <<
										(partition->IsClosed() ? "" : "NOT ") << "Closed)" << endl;
									
									//size_t Count = 0;
									for(MDObjectList::iterator p_it = partition->AllMetadata.begin(); p_it != partition->AllMetadata.end(); ++p_it)
									{
										PrintMDO(*p_it, "\t\t");
										
										//cout << "\t\t" << mdata->FullName() << " (" << mdata->GetType() << ", " <<
										//	mdata->GetRefType() << ", " << mdata->GetUL()->GetString() << ", " <<
										//	mdata->GetTag() << ")" << endl;
										
										//p_it++;
									}
								}
								else
									cout << "\tNo metadata in partition" << endl;
							}
							else
								cout << "\tFailed to read partition" << endl;
							
							//it++;
						}
					}
					else
					{
						cerr << "Error reading RIP" << endl;
						return -1;
					}

					}
					else
						cout << "\tFailed to read partition" << endl;*/
					
					
					file->Close();
				}
				else
				{
					cerr << "Error opening file" << endl;
					return -1;
				}
				
				MoxMxf::DeleteIOStream(file_handle);
			}
			catch(...)
			{
				cerr << "Exception while opening file" << endl;
				return -1;
			}
		}
		else
		{
			cerr << "Usage: mxfparse <file>" << endl;
			return -1;
		}
	}
	catch(...)
	{
		cerr << "Unhandled exception" << endl;
		return -1;
	}
	
	return 0;
}



