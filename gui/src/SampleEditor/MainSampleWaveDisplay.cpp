/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "config.h"

#include <hydrogen/sample.h>
#include <hydrogen/Song.h>
#include <hydrogen/instrument.h>
using namespace H2Core;

#include "MainSampleWaveDisplay.h"
#include "../Skin.h"


MainSampleWaveDisplay::MainSampleWaveDisplay(QWidget* pParent)
 : QWidget( pParent )
 , Object( "MainSampleWaveDisplay" )
{
	setAttribute(Qt::WA_NoBackground);

	//INFOLOG( "INIT" );
	int w = 624;
	int h = 265;
	resize( w, h );

	bool ok = m_background.load( Skin::getImagePath() + "/waveDisplay/mainsamplewavedisplay.png" );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap" );
	}

	m_pPeakDatal = new int[ w ];
	m_pPeakDatar = new int[ w ];

}




MainSampleWaveDisplay::~MainSampleWaveDisplay()
{
	//INFOLOG( "DESTROY" );

	delete[] m_pPeakDatal;
	delete[] m_pPeakDatar;
}



void MainSampleWaveDisplay::paintEvent(QPaintEvent *ev)
{
	QPainter painter( this );
	painter.setRenderHint( QPainter::Antialiasing );
	painter.drawPixmap( ev->rect(), m_background, ev->rect() );

	painter.setPen( QColor( 230, 230, 230 ) );
	int VCenterl = height() / 4;
	int VCenterr = height() / 4 + height() / 2;

	for ( int x = 25; x < width() -25; x++ ) {
		painter.drawLine( x, VCenterl, x, m_pPeakDatal[x] + VCenterl );
		painter.drawLine( x, VCenterl, x, -m_pPeakDatal[x] + VCenterl );
		painter.drawLine( x, VCenterr, x, m_pPeakDatar[x] + VCenterr );
		painter.drawLine( x, VCenterr, x, -m_pPeakDatar[x] + VCenterr );
		
	}

	int VHight = height();
	painter.setPen( QPen( QColor( 255, 255, 255 ), 1, Qt::DotLine ) );
	painter.drawLine( 23, 4, 23, height() -4 );
	painter.drawLine( width() -23, 4,width() -23, height() -4 );
	painter.setPen( QPen( QColor( 255, 255, 255 ), 1, Qt::SolidLine ) );
	painter.drawLine( 0, VCenterl, width(),VCenterl );
	painter.drawLine( 0, VCenterr, width(),VCenterr );

}



void MainSampleWaveDisplay::updateDisplay( QString filename )
{

	Sample *pNewSample = Sample::load( filename );

	if ( pNewSample ) {

		int nSampleLenght = pNewSample->get_n_frames();
		float nScaleFactor = nSampleLenght / width();

		float fGain = height() / 4.0 * 1.0;

		float *pSampleDatal = pNewSample->get_data_l();

		int nSamplePos =0;
		int nVall;
		for ( int i = 0; i < width(); ++i ){
			nVall = 0;
			for ( int j = 0; j < nScaleFactor; ++j ) {
				if ( j < nSampleLenght ) {
					int newVall = (int)( pSampleDatal[ nSamplePos ] * fGain );
					if ( newVall > nVall ) {
						nVall = newVall;
					}
				}
				++nSamplePos;
			}
			m_pPeakDatal[ i ] = nVall;
		}

		float *pSampleDatar = pNewSample->get_data_r();

		nSamplePos = 0;
		int nValr;
		for ( int i = 0; i < width(); ++i ){
			nValr = 0;
			for ( int j = 0; j < nScaleFactor; ++j ) {
				if ( j < nSampleLenght ) {
					int newValr = (int)( pSampleDatar[ nSamplePos ] * fGain );
					if ( newValr > nValr ) {
						nValr = newValr;
					}
				}
				++nSamplePos;
			}
			m_pPeakDatar[ i ] = nValr;
		}
	}

	delete pNewSample;
	update();

}

