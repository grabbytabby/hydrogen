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

#include <core/Hydrogen.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>


using namespace H2Core;

#include <QTimer>
#include <QPainter>

#include "DrumPatternEditor.h"
#include "PatternEditorRuler.h"
#include "PatternEditorPanel.h"
#include "NotePropertiesRuler.h"
#include "../HydrogenApp.h"
#include "../Skin.h"


PatternEditorRuler::PatternEditorRuler( QWidget* parent )
 : QWidget( parent )
 {
	setAttribute(Qt::WA_OpaquePaintEvent);

	//infoLog( "INIT" );

	Preferences *pPref = Preferences::get_instance();

	QColor backgroundColor( pPref->getColorTheme()->m_patternEditor_backgroundColor );

	m_pPattern = nullptr;
	m_fGridWidth = Preferences::get_instance()->getPatternEditorGridWidth();

	m_nRulerWidth = 20 + m_fGridWidth * ( MAX_NOTES * 4 );
	m_nRulerHeight = 25;

	resize( m_nRulerWidth, m_nRulerHeight );

	bool ok = m_tickPosition.load( Skin::getImagePath() + "/patternEditor/tickPosition.png" );
	if( ok == false ){
		ERRORLOG( "Error loading pixmap " );
	}

	m_pBackground = new QPixmap( m_nRulerWidth, m_nRulerHeight );
	m_pBackground->fill( backgroundColor );

	m_pTimer = new QTimer(this);
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updateEditor()));

	HydrogenApp::get_instance()->addEventListener( this );
}



PatternEditorRuler::~PatternEditorRuler() {
	//infoLog( "DESTROY");
}



void PatternEditorRuler::updateStart(bool start) {
	if (start) {
		m_pTimer->start(50);	// update ruler at 20 fps
	}
	else {
		m_pTimer->stop();
	}
}



void PatternEditorRuler::showEvent ( QShowEvent *ev )
{
	UNUSED( ev );
	updateEditor();
	updateStart(true);
}



void PatternEditorRuler::hideEvent ( QHideEvent *ev )
{
	UNUSED( ev );
	updateStart(false);
}



void PatternEditorRuler::updateEditor( bool bRedrawAll )
{
	static int oldNTicks = 0;

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	//Do not redraw anything if Export is active.
	//https://github.com/hydrogen-music/hydrogen/issues/857	
	if( pHydrogen->getIsExportSessionActive() ) {
		return;
	}
	
	PatternList *pPatternList = pHydrogen->getSong()->getPatternList();
	int nSelectedPatternNumber = pHydrogen->getSelectedPatternNumber();
	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->size() )  ) {
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
	}
	else {
		m_pPattern = nullptr;
	}


	bool bActive = false;	// is the pattern playing now?

	if ( pHydrogen->getMode() == Song::Mode::Song &&
		 pHydrogen->isPatternEditorLocked() ) {
		// In case the pattern editor is locked we will always display
		// the position tick. Even if no pattern is set at all.
		bActive = true;
	} else {
		/* 
		 * Lock audio engine to make sure pattern list does not get
		 * modified / cleared during iteration 
		 */
		pAudioEngine->lock( RIGHT_HERE );

		PatternList *pList = pAudioEngine->getPlayingPatterns();
		for (uint i = 0; i < pList->size(); i++) {
			if ( m_pPattern == pList->get(i) ) {
				bActive = true;
				break;
			}
		}

		pAudioEngine->unlock();
	}

	if ( ( pAudioEngine->getState() == H2Core::AudioEngine::State::Playing ) && bActive ) {
		m_nTicks = pAudioEngine->getPatternTickPosition();
	}
	else {
		m_nTicks = -1;	// hide the tickPosition
	}


	if (oldNTicks != m_nTicks) {
		// redraw all
		bRedrawAll = true;
	}
	oldNTicks = m_nTicks;

	if (bRedrawAll) {
		update( 0, 0, width(), height() );
	}
}



void PatternEditorRuler::paintEvent( QPaintEvent *ev)
{
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pDrumPatternEditor = pHydrogenApp->getPatternEditorPanel()->getDrumPatternEditor();
	auto pPref = H2Core::Preferences::get_instance();

	if (!isVisible()) {
		return;
	}

	QPainter painter(this);

	QColor backgroundColor( pPref->getColorTheme()->m_patternEditor_alternateRowColor.darker( 120 ) );
	QColor textColor = pPref->getColorTheme()->m_patternEditor_textColor;
	QColor lineColor = pPref->getColorTheme()->m_patternEditor_lineColor;
	
	painter.fillRect( QRect( 1, 1, width() - 2, height() - 2 ), backgroundColor );

	painter.setPen( QColor( 35, 39, 51 ) );
	painter.drawLine( 0, 0, width(), 0 );
	painter.drawLine( 0, height(), width(), height() );

	// gray background for unusable section of pattern
	int nNotes = MAX_NOTES;
	if ( m_pPattern != nullptr ) {
		nNotes = m_pPattern->get_length();
	}
	int nXStart = 20 + nNotes * m_fGridWidth;
	if ( (m_nRulerWidth - nXStart) != 0 ) {
		painter.fillRect( nXStart, 0, m_nRulerWidth - nXStart, m_nRulerHeight,
						  pPref->getColorTheme()->m_midLightColor );
	}

	// numbers

	QFont font( pPref->getApplicationFontFamily(), getPointSize( pPref->getFontSize() ) );
	painter.setFont(font);
	painter.drawLine( 0, 0, m_nRulerWidth, 0 );
	painter.drawLine( 0, m_nRulerHeight - 1, m_nRulerWidth - 1, m_nRulerHeight - 1);

	uint nQuarter = 48;

	// Draw numbers and quarter ticks
	for ( int ii = 0; ii < 64 ; ii += 4 ) {
		int nText_x = pDrumPatternEditor->getMargin() +
			nQuarter / 4 * ii * m_fGridWidth;
		painter.setPen( textColor );
		painter.drawLine( nText_x, height() - 13, nText_x, height() - 1 );
		painter.drawText( nText_x + 3, 0, 60, m_nRulerHeight,
						  Qt::AlignVCenter | Qt::AlignLeft,
						  QString("%1").arg(ii / 4 + 1) );
	}

	// Draw remaining ticks
	int nMaxX = m_fGridWidth * nNotes + pDrumPatternEditor->getMargin();

	float fStep;
	if ( pDrumPatternEditor->isUsingTriplets() ) {
		fStep = 4 * MAX_NOTES / ( 3 * pDrumPatternEditor->getResolution() )
			* m_fGridWidth;
	} else {
		fStep = 4 * MAX_NOTES / ( 4 * pDrumPatternEditor->getResolution() )
			* m_fGridWidth;
	}
	for ( float xx = pDrumPatternEditor->getMargin(); xx < nMaxX; xx += fStep ) {
		painter.drawLine( xx, height() - 5, xx, height() - 1 );
	}

	// draw tickPosition
	if (m_nTicks != -1) {
		uint x = (uint)( 20 + m_nTicks * m_fGridWidth - 5 - 11 / 2.0 );
		painter.drawPixmap( QRect( x, height() / 2, 11, 8 ), m_tickPosition, QRect( 0, 0, 11, 8 ) );

	}

	// draw cursor
	if ( ( pDrumPatternEditor->hasFocus() ||
		   pHydrogenApp->getPatternEditorPanel()->getVelocityEditor()->hasFocus() ||
		   pHydrogenApp->getPatternEditorPanel()->getPanEditor()->hasFocus() ||
		   pHydrogenApp->getPatternEditorPanel()->getLeadLagEditor()->hasFocus() ||
		   pHydrogenApp->getPatternEditorPanel()->getNoteKeyEditor()->hasFocus() ||
		   pHydrogenApp->getPatternEditorPanel()->getProbabilityEditor()->hasFocus() ||
		   pHydrogenApp->getPatternEditorPanel()->getPianoRollEditor()->hasFocus() ) &&
		! pHydrogenApp->hideKeyboardCursor() ) {

		int nCursorX = m_fGridWidth *
			pHydrogenApp->getPatternEditorPanel()->getCursorPosition() +
			pDrumPatternEditor->getMargin() - 4 -
			m_fGridWidth * 5;

		// Middle line to indicate the selected tick
		painter.setPen( Qt::black );
		painter.drawLine( nCursorX + m_fGridWidth * 5 + 4, height() - 6,
						  nCursorX + m_fGridWidth * 5 + 4, height() - 13 );

		QPen pen;
		pen.setWidth( 2 );
		painter.setPen( pen );
		painter.setRenderHint( QPainter::Antialiasing );
		painter.drawLine( nCursorX, 3, nCursorX + m_fGridWidth * 10 + 8, 3 );
		painter.drawLine( nCursorX, 4, nCursorX, 5 );
		painter.drawLine( nCursorX + m_fGridWidth * 10 + 8, 4,
						  nCursorX + m_fGridWidth * 10 + 8, 5 );
		painter.drawLine( nCursorX, height() - 5,
						  nCursorX + m_fGridWidth * 10 + 8, height() - 5 );
		painter.drawLine( nCursorX, height() - 7,
						  nCursorX, height() - 6 );
		painter.drawLine( nCursorX + m_fGridWidth * 10 + 8, height() - 6,
						  nCursorX + m_fGridWidth * 10 + 8, height() - 7 );
	}
}

void PatternEditorRuler::zoomIn()
{
	
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( m_fGridWidth >= 3 ){
		m_fGridWidth *= 2;
	} else {
		m_fGridWidth *= 1.5;
	}
	m_nRulerWidth = 20 + m_fGridWidth * ( MAX_NOTES * 4 );
	resize(  QSize(m_nRulerWidth, m_nRulerHeight ));
	delete m_pBackground;
	m_pBackground = new QPixmap( m_nRulerWidth, m_nRulerHeight );
	QColor backgroundColor( pPref->getColorTheme()->m_patternEditor_backgroundColor );
	m_pBackground->fill( backgroundColor );
	update();
}


void PatternEditorRuler::zoomOut()
{
	
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( m_fGridWidth > 1.5 ) {
		if ( m_fGridWidth > 3 ){
			m_fGridWidth /= 2;
		} else {
			m_fGridWidth /= 1.5;
		}
		m_nRulerWidth = 20 + m_fGridWidth * ( MAX_NOTES * 4 );
		resize( QSize(m_nRulerWidth, m_nRulerHeight) );
		delete m_pBackground;
		m_pBackground = new QPixmap( m_nRulerWidth, m_nRulerHeight );
		QColor backgroundColor( pPref->getColorTheme()->m_patternEditor_backgroundColor );
		m_pBackground->fill( backgroundColor );
		update();
	}
}


void PatternEditorRuler::selectedPatternChangedEvent()
{
	updateEditor( true );
}

void PatternEditorRuler::onPreferencesChanged( H2Core::Preferences::Changes changes ) {
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {
		update( 0, 0, width(), height() );
	}
}
