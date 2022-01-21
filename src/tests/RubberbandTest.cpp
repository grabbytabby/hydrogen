/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include "core/Basics/Sample.h"

#include "sndfile.h"
#ifdef H2CORE_HAVE_RUBBERBAND
#include <rubberband/RubberBandStretcher.h>
#define RUBBER_SAMPLE_PATH "/usr/local/share/hydrogen/data/drumkits/GMkit/cym_Jazz.flac"

void rubberband_test( const QString& sample_path ) {
	// set rubber band options
	int debug = 1;
	float pitch = 1.5946;
	float time_ratio =  1.83199;
	RubberBand::RubberBandStretcher::Options options = 131088; //RubberBand::RubberBandStretcher::DefaultOptions;
	
	// load a sample
	auto sample = H2Core::Sample::load( sample_path );
	if( sample==nullptr ) {
		___ERRORLOG( QString( "unable to load %1" ).arg( sample_path ) );
		return;
	}
	___DEBUGLOG( QString( "input sample\n\tfilename\t: %1\n\tframes\t\t: %2\n\tsample rate\t: %3" )
	             .arg( sample->get_filename().toLocal8Bit().data() )
	             .arg( sample->get_frames() )
	             .arg( sample->get_sample_rate() )
	             );
	
	sample->write( "/tmp/before.wav" );
	
	// setup rubberband
	RubberBand::RubberBandStretcher* rubber = new RubberBand::RubberBandStretcher( sample->get_sample_rate(), 2, options, time_ratio, pitch );
	rubber->setDebugLevel( debug );
	rubber->setExpectedInputDuration( sample->get_frames() );
	___DEBUGLOG( QString( "rubberband options\n\tdebug\t\t: %1\n\toptions\t\t: %2\n\ttime ratio\t: %3\n\tpitch\t\t: %4" ).arg( debug ).arg( options ).arg( time_ratio ).arg( pitch ) );
	___DEBUGLOG( QString( "minimum sample required: %1" ).arg( rubber->getSamplesRequired() ) );
	
	// study
	float* ibuf[2];
	int studied = 0;
	___DEBUGLOG( "Study ..." );
	/*
	while( studied < sample->get_frames() ) {
		ibuf[0] = &sample->get_data_l()[studied];
		ibuf[1] = &sample->get_data_r()[studied];
		bool final = (studied + block_size >= sample->get_frames());
		int ibs = (final ? (sample->get_frames()-studied) : block_size );
		//___DEBUGLOG( QString(" ibs : %1").arg( ibs ) );
		rubber->study( ibuf, ibs, final );
		studied += ibs;
		if( final ) break;
	}
	*/
	studied = sample->get_frames();
	ibuf[0] = sample->get_data_l();
	ibuf[1] = sample->get_data_r();
	rubber->study( ibuf, studied, true );
	___DEBUGLOG( QString("done.\n  %1 frames studied.").arg( studied ) );
	
	// buffers
	float* obuf[2];
	int out_buffer_size = (int)(sample->get_frames()*time_ratio)+1000;
	float* out_data_l = new float[ out_buffer_size ];
	float* out_data_r = new float[ out_buffer_size ];
	int processed = 0;
	int retrieved = 0;
	int available = 0;
	int buffer_free = out_buffer_size;
	___DEBUGLOG( "Process ..." );
	/*
	while( processed < sample->get_frames() ) {
		ibuf[0] = &sample->get_data_l()[processed];
		ibuf[1] = &sample->get_data_r()[processed];
		bool final = (processed + block_size >= sample->get_frames());
		int ibs = (final ? (sample->get_frames()-processed) : block_size );
		//___DEBUGLOG( QString(" ibs : %1").arg( ibs ) );
		rubber->process( ibuf, ibs, final );
		processed += ibs;
		if( final ) break;
		// retrieve data
		while( (available=rubber->available())>0 && buffer_free>0 ) {
			obuf[0] = &out_data_l[retrieved];
			obuf[1] = &out_data_r[retrieved];
			//___DEBUGLOG( QString( "  available frames %1" ).arg( available ) );
			int n = rubber->retrieve( obuf, available);
			retrieved += n;
			buffer_free -= n;
			//___DEBUGLOG( QString( "  received frames %1" ).arg( n ) );
		}
	}
	*/
	processed = sample->get_frames();
	ibuf[0] = sample->get_data_l();
	ibuf[1] = sample->get_data_r();
	rubber->process( ibuf, processed, true );
	
	// retrieve last frames
	while( (available=rubber->available())>0 && buffer_free>0 ) {
		obuf[0] = &out_data_l[retrieved];
		obuf[1] = &out_data_r[retrieved];
		//___DEBUGLOG( QString( "  available frames %1" ).arg( available ) );
		int n = rubber->retrieve( obuf, available);
		retrieved += n;
		buffer_free -= n;
		//___DEBUGLOG( QString( "  received frames %1" ).arg( n ) );
	}
	___DEBUGLOG( QString( "done.\n  %1 frames processed\n  %2 frames retrieved [ %3 expected ]" ).arg( processed ).arg( retrieved ).arg( sample->get_frames()*time_ratio ) );
	
	// final data buffers
	float* data_l = new float[ retrieved ];
	float* data_r = new float[ retrieved ];
	for( int i=0; i<retrieved; i++) {
		data_r[i] = data_l[i] = 0.5;
	}
       
	// feed final data buffers
	memcpy( data_l, out_data_l, retrieved*sizeof(float) );
	memcpy( data_r, out_data_r, retrieved*sizeof(float) );
	
	// new sample
	auto sample2 = std::make_shared<H2Core::Sample>( "/tmp/after.wav", retrieved, sample->get_sample_rate(), data_l, data_r );
	sample2->write( "/tmp/after.wav" );
	
	// clean
	delete rubber;
	delete[] out_data_l;
	delete[] out_data_r;
}
#else
void rubberband_test( const QString& sample_path ) {
    ___ERRORLOG("RUBBERBAND LIBRARY NOT AVAILABLE");
}
#endif
