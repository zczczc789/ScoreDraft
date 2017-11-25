#include "Instrument.h"
#include "Note.h"
#include "TrackBuffer.h"
#include <memory.h>
#include <cmath>
#include <vector>

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

NoteBuffer::NoteBuffer()
{
	m_sampleNum=0;
	m_data=0;
}

NoteBuffer::~NoteBuffer()
{
	delete[] m_data;
}

void NoteBuffer::Allocate()
{
	delete[] m_data;
	m_data=new float[m_sampleNum];
}

class NoteBufferItem
{
public:
	Note m_note;
	NoteBuffer m_noteBuffer;
};

class NoteTable : public std::vector<NoteBufferItem*> {};

Instrument::Instrument()
{
	m_accelerate=true;
	//m_accelerate=false;
	m_NoteTable=new NoteTable;
}

Instrument::~Instrument()
{
	unsigned i;
	for (i=0;i<m_NoteTable->size();i++)
	{
		delete m_NoteTable->at(i);
	}
	delete m_NoteTable;
}

void Instrument::Silence(unsigned numOfSamples, NoteBuffer* noteBuf)
{
	noteBuf->m_sampleNum=numOfSamples;
	noteBuf->Allocate();
	memset(noteBuf->m_data,0,sizeof(float)*numOfSamples);
}

void Instrument::GenerateNoteWave(unsigned numOfSamples, float sampleFreq, NoteBuffer* noteBuf)
{
	Silence(numOfSamples,noteBuf);
}

void Instrument::PlayNote(TrackBuffer& buffer, const Note& aNote, unsigned tempo, float RefFreq)
{
	NoteBuffer l_noteBuf;
	NoteBuffer *noteBuf=&l_noteBuf;

	float fduration=fabsf((float)(aNote.m_duration*60))/(float)(tempo*48);
	unsigned numOfSamples=(unsigned)(buffer.Rate()*fduration);

	bool bufferFilled=false;
	if (aNote.m_freq_rel<0.0f)
	{
		if (aNote.m_duration>0) 
		{
			Silence(numOfSamples, noteBuf);
			bufferFilled=true;
		}
		else if (aNote.m_duration<0)
		{
			buffer.SeekSample(-min((long)numOfSamples,buffer.Tell()),SEEK_CUR);
			return;
		}
		else return;
	}

	if (!bufferFilled)
	{
		if (m_accelerate)
		{
			unsigned i;
			for (i=0;i<m_NoteTable->size();i++)
			{
				Note& tabNote=m_NoteTable->at(i)->m_note;
				if (tabNote.m_duration == aNote.m_duration && fabsf(tabNote.m_freq_rel - aNote.m_freq_rel)<0.01f)
				{
					noteBuf=&(m_NoteTable->at(i)->m_noteBuffer);
					bufferFilled=true;
					break;
				}
			}
			if (i==m_NoteTable->size())
			{
				NoteBufferItem* nbi=new NoteBufferItem;
				nbi->m_note=aNote;
				noteBuf=&nbi->m_noteBuffer;
				m_NoteTable->push_back(nbi);
			}
		}

		if (!bufferFilled)
		{
			float freq = RefFreq*aNote.m_freq_rel;
			float sampleFreq=freq/(float)buffer.Rate();				
			GenerateNoteWave(numOfSamples, sampleFreq, noteBuf);
		}
	}
	
	buffer.WriteBlend(noteBuf->m_sampleNum,noteBuf->m_data);
	buffer.SeekSample(numOfSamples-noteBuf->m_sampleNum,SEEK_CUR);
	
}

void Instrument::PlayNotes(TrackBuffer& buffer, const NoteSequence& seq, unsigned tempo, float RefFreq)
{
	unsigned i;
	int prog=0;
	for (i=0;i<seq.size();i++)
	{
		int newprog = (i + 1) * 10 / seq.size();
		if (newprog>prog)
		{
			printf("-");
			prog=newprog;
		}
			
		PlayNote(buffer,seq[i],tempo,RefFreq);
	}
	printf("\n");
}