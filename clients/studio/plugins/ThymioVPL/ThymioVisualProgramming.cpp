#include <QCloseEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSizePolicy>
#include <QFileDialog>
#include <QFile>
#include <QDomElement>
#include <QDesktopWidget>
#include <QApplication>
#include <QScrollBar>
#include <QAction>
#include <QUrl>
#include <QDesktopServices>
#include <QtDebug>

#include "ThymioVisualProgramming.h"
#include "Card.h"
#include "EventCards.h"
#include "ActionCards.h"
#include "Scene.h"
#include "Buttons.h"

#include "../../TargetModels.h"

using namespace std;

namespace Aseba { namespace ThymioVPL
{
	// Visual Programming
	ThymioVisualProgramming::ThymioVisualProgramming(DevelopmentEnvironmentInterface *_de, bool showCloseButton):
		de(_de)
	{
		// Create the gui ...
		setWindowTitle(tr("Thymio Visual Programming Language"));
		setMinimumSize(QSize(400,400));
		
		mainLayout = new QVBoxLayout(this);
		
		toolBar = new QToolBar();
		mainLayout->addWidget(toolBar);

		newButton = new QToolButton();
		newButton->setIcon(QIcon(":/images/filenew.svgz"));
		newButton->setToolTip(tr("New"));
		toolBar->addWidget(newButton);
		
		openButton = new QToolButton();
		openButton->setIcon(QIcon(":/images/fileopen.svgz"));
		openButton->setToolTip(tr("Open"));
		toolBar->addWidget(openButton);
		
		saveButton = new QToolButton();
		saveButton->setIcon(QIcon(":/images/save.svgz"));
		saveButton->setToolTip(tr("Save"));
		toolBar->addWidget(saveButton);
		
		saveAsButton = new QToolButton();
		saveAsButton->setIcon(QIcon(":/images/saveas.svgz"));
		saveAsButton->setToolTip(tr("Save as"));
		toolBar->addWidget(saveAsButton);
		toolBar->addSeparator();

		runButton = new QToolButton();
		runButton->setIcon(QIcon(":/images/play.svgz"));
		runButton->setToolTip(tr("Load & Run"));
		toolBar->addWidget(runButton);

		stopButton = new QToolButton();
		stopButton->setIcon(QIcon(":/images/stop1.svgz"));
		stopButton->setToolTip(tr("Stop"));
		toolBar->addWidget(stopButton);
		toolBar->addSeparator();
	
		colorComboButton = new QComboBox();
		colorComboButton->setToolTip(tr("Color scheme"));
		setColors(colorComboButton);
		toolBar->addWidget(colorComboButton);
		toolBar->addSeparator();

		advancedButton = new QToolButton();
		advancedButton->setIcon(QIcon(":/images/vpl_advanced_mode.svgz"));
		advancedButton->setToolTip(tr("Advanced mode"));
		toolBar->addWidget(advancedButton);
		toolBar->addSeparator();
		
		helpButton = new QToolButton();
		QAction* action = new QAction(helpButton);
		action->setIcon(QIcon(":/images/info.svgz"));
		action->setToolTip(tr("Help"));
		action->setData(QUrl(tr("http://aseba.wikidot.com/en:thymiovpl")));
		connect(action, SIGNAL(triggered()), this, SLOT(openToUrlFromAction()));
		helpButton->setDefaultAction(action);
		toolBar->addWidget(helpButton);

		if (showCloseButton)
		{
			toolBar->addSeparator();
			quitButton = new QToolButton();
			quitButton->setIcon(QIcon(":/images/exit.svgz"));
			quitButton->setToolTip(tr("Quit"));
			toolBar->addWidget(quitButton);
			connect(quitButton, SIGNAL(clicked()), this, SLOT(closeFile()));
		}
		else
			quitButton = 0;
		
		connect(newButton, SIGNAL(clicked()), this, SLOT(newFile()));
		connect(openButton, SIGNAL(clicked()), this, SLOT(openFile()));
		connect(saveButton, SIGNAL(clicked()), this, SLOT(save()));
		connect(saveAsButton, SIGNAL(clicked()), this, SLOT(saveAs()));
		connect(colorComboButton, SIGNAL(currentIndexChanged(int)), this, SLOT(setColorScheme(int)));
		
		connect(runButton, SIGNAL(clicked()), this, SLOT(run()));
		connect(stopButton, SIGNAL(clicked()), this, SLOT(stop()));
		connect(advancedButton, SIGNAL(clicked()), this, SLOT(toggleAdvancedMode()));
		
		horizontalLayout = new QHBoxLayout();
		mainLayout->addLayout(horizontalLayout);
		
		// events
		eventsLayout = new QVBoxLayout();

		CardButton *buttonsButton = new CardButton("button");
		CardButton *proxButton = new CardButton("prox");
		CardButton *proxGroundButton = new CardButton("proxground");
		CardButton *tapButton = new CardButton("tap");
		CardButton *clapButton = new CardButton("clap");

		eventButtons.push_back(buttonsButton);
		eventButtons.push_back(proxButton);
		eventButtons.push_back(proxGroundButton);
		eventButtons.push_back(tapButton);
		eventButtons.push_back(clapButton);
		
		eventsLabel = new QLabel(tr("<b>Events</b>"));
		eventsLabel ->setStyleSheet("QLabel { font-size: 10pt; }");
		eventsLayout->setAlignment(Qt::AlignTop);
		eventsLayout->setSpacing(0);
		CardButton* button;
		eventsLayout->addWidget(eventsLabel);
		foreach (button, eventButtons)
		{
			eventsLayout->addItem(new QSpacerItem(0,10));
			eventsLayout->addWidget(button);
		}
		
		horizontalLayout->addLayout(eventsLayout);
		
		sceneLayout = new QVBoxLayout();

		// compilation
		compilationResultImage = new QLabel();
		compilationResult = new QLabel(tr("Compilation success."));
		compilationResult->setStyleSheet("QLabel { font-size: 10pt; }");
		
		compilationResultImage->setPixmap(QPixmap(QString(":/images/ok.png")));
		compilationResult->setWordWrap(true);
		compilationResult->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);

		compilationResultLayout = new QHBoxLayout;
		compilationResultLayout->addWidget(compilationResultImage);  
		compilationResultLayout->addWidget(compilationResult,10000);
		sceneLayout->addLayout(compilationResultLayout);

		// scene
		scene = new Scene(this);
		view = new QGraphicsView(scene);
		view->setRenderHint(QPainter::Antialiasing);
		view->setAcceptDrops(true);
		view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		sceneLayout->addWidget(view);
		Q_ASSERT(scene->pairsEnd() - scene->pairsBegin());
		view->centerOn(*scene->pairsBegin());
		
		connect(scene, SIGNAL(contentRecompiled()), this, SLOT(processCompilationResult()));

		horizontalLayout->addLayout(sceneLayout);
     
		// actions
		actionsLayout = new QVBoxLayout();

		CardButton *moveButton = new CardButton("move");
		CardButton *colorTopButton = new CardButton("colortop");
		CardButton *colorBottomButton = new CardButton("colorbottom");
		CardButton *soundButton = new CardButton("sound");
		CardButton *memoryButton = new CardButton("statefilter");
		actionsLabel = new QLabel(tr("<b>Actions</b>"));
		actionsLabel ->setStyleSheet("QLabel { font-size: 10pt; }");
		
		actionButtons.push_back(moveButton);
		actionButtons.push_back(colorTopButton);
		actionButtons.push_back(colorBottomButton);
		actionButtons.push_back(soundButton);
		actionButtons.push_back(memoryButton);
		
		actionsLayout->setAlignment(Qt::AlignTop);
		actionsLayout->setSpacing(0);
		actionsLayout->addWidget(actionsLabel);
		foreach (button, actionButtons)
		{
			actionsLayout->addItem(new QSpacerItem(0,10));
			actionsLayout->addWidget(button);
		}
		
		memoryButton->hide(); // memory

		horizontalLayout->addLayout(actionsLayout);
		
		// make connections
		connect(buttonsButton, SIGNAL(clicked()),this,SLOT(addButtonsEvent()));
		connect(proxButton, SIGNAL(clicked()), this, SLOT(addProxEvent()));
		connect(proxGroundButton, SIGNAL(clicked()), this, SLOT(addProxGroundEvent()));
		connect(tapButton, SIGNAL(clicked()), this, SLOT(addTapEvent()));
		connect(clapButton, SIGNAL(clicked()), this, SLOT(addClapEvent()));

		connect(moveButton, SIGNAL(clicked()), this, SLOT(addMoveAction()));
		connect(colorTopButton, SIGNAL(clicked()), this, SLOT(addColorTopAction()));	
		connect(colorBottomButton, SIGNAL(clicked()), this, SLOT(addColorBottomAction()));
		connect(soundButton, SIGNAL(clicked()), this, SLOT(addSoundAction()));
		connect(memoryButton, SIGNAL(clicked()), this, SLOT(addMemoryAction()));
		
		setWindowModality(Qt::ApplicationModal);
	}
	
	ThymioVisualProgramming::~ThymioVisualProgramming()
	{
	}
	
	void ThymioVisualProgramming::openToUrlFromAction() const
	{
		const QAction *action(reinterpret_cast<QAction *>(sender()));
		QDesktopServices::openUrl(action->data().toUrl());
	}
	
	void ThymioVisualProgramming::setColors(QComboBox *button)
	{
		eventColors.push_back(QColor(0,191,255)); actionColors.push_back(QColor(218,112,214));
		eventColors.push_back(QColor(155,48,255)); actionColors.push_back(QColor(159,182,205));
		eventColors.push_back(QColor(67,205,128)); actionColors.push_back(QColor(0,197,205)); 
		eventColors.push_back(QColor(255,215,0)); actionColors.push_back(QColor(255,99,71));
		eventColors.push_back(QColor(255,97,3)); actionColors.push_back(QColor(142,56,142));
		eventColors.push_back(QColor(125,158,192)); actionColors.push_back(QColor(56,142,142)); 

		if( button )
		{
			for(int i=0; i<eventColors.size(); ++i)
				button->addItem(drawColorScheme(eventColors.at(i), actionColors.at(i)), "");
		}
	}
	
	QPixmap ThymioVisualProgramming::drawColorScheme(QColor color1, QColor color2)
	{
		QPixmap pixmap(128,58);
		pixmap.fill(Qt::transparent);
		QPainter painter(&pixmap);
		
		painter.setBrush(color1);
		painter.drawRoundedRect(0,0,54,54,4,4);
		
		painter.setBrush(color2);
		painter.drawRoundedRect(66,0,54,54,4,4);
		
		return pixmap;
	}
	
	QWidget* ThymioVisualProgramming::createMenuEntry()
	{
		QPushButton *vplButton = new QPushButton(tr("Launch VPL"));
		connect(vplButton, SIGNAL(clicked()), SLOT(showVPLModal()));
		return vplButton;
	}
	
	void ThymioVisualProgramming::closeAsSoonAsPossible()
	{
		advancedButton->setEnabled(true);
		actionButtons.last()->hide(); // state filter card
		scene->reset();
		close();
	}

	void ThymioVisualProgramming::showVPLModal()
	{
		if (de->newFile())
			show();
	}
	
	void ThymioVisualProgramming::newFile()
	{
		if (scene->isEmpty() || warningDialog()) 
		{
			scene->reset();
			toggleAdvancedMode(false);
		}
	}

	void ThymioVisualProgramming::openFile()
	{
		if( scene->isEmpty() || warningDialog() ) 
			de->openFile();
	}
	
	bool ThymioVisualProgramming::save()
	{
		return de->saveFile(false);
	}
	
	bool ThymioVisualProgramming::saveAs()
	{
		return de->saveFile(true);
	}

	bool ThymioVisualProgramming::closeFile()
	{
		if( scene->isEmpty() || warningDialog() ) 
		{
			advancedButton->setEnabled(true);
			actionButtons.last()->hide(); // state button
			scene->reset();
			close();
			return true;
		}
		else
			return false;
	}
	
	void ThymioVisualProgramming::setColorScheme(int index)
	{
		scene->setColorScheme(eventColors.at(index), actionColors.at(index));
		
		for(QList<CardButton*>::iterator itr(eventButtons.begin());
			itr != eventButtons.end(); ++itr)
			(*itr)->changeButtonColor(eventColors.at(index));

		for(QList<CardButton*>::iterator itr(actionButtons.begin());
			itr != actionButtons.end(); ++itr)
			(*itr)->changeButtonColor(actionColors.at(index));
	}

	void ThymioVisualProgramming::run()
	{
		if(runButton->isEnabled())
			de->loadAndRun();
	}

	void ThymioVisualProgramming::stop()
	{
		de->stop();
		const unsigned leftSpeedVarPos = de->getVariablesModel()->getVariablePos("motor.left.target");
		de->setVariableValues(leftSpeedVarPos, VariablesDataVector(1, 0));
		const unsigned rightSpeedVarPos = de->getVariablesModel()->getVariablePos("motor.right.target");
		de->setVariableValues(rightSpeedVarPos, VariablesDataVector(1, 0));
	}
	
	void ThymioVisualProgramming::toggleAdvancedMode()
	{
		toggleAdvancedMode(!scene->getAdvanced());
	}
	
	void ThymioVisualProgramming::toggleAdvancedMode(bool advanced, bool force)
	{
		if (advanced)
		{
			advancedButton->setIcon(QIcon(":/images/vpl_simple_mode.svgz"));
			actionButtons.last()->show(); // state button
			scene->setAdvanced(true);
		}
		else
		{
			bool doClear(true);
			if (!force && scene->isAnyStateFilter())
			{
				if (QMessageBox::warning(this, tr("Returning to simple mode"),
					tr("You are currently using states. Returning to simple mode will discard any state filter or state setting card.<p>Are you sure you want to continue?"),
				QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::No)
					doClear = false;
			}
			
			if (doClear)
			{
				advancedButton->setIcon(QIcon(":/images/vpl_advanced_mode.svgz"));
				actionButtons.last()->hide(); // state button
				scene->setAdvanced(false);
			}
		}
	}
	
	void ThymioVisualProgramming::closeEvent ( QCloseEvent * event )
	{
		if (!closeFile())
			event->ignore();
	}
	
	bool ThymioVisualProgramming::warningDialog()
	{
		if(!scene->isModified())
			return true;
		
		QMessageBox msgBox;
		msgBox.setWindowTitle(tr("Warning"));
		msgBox.setText(tr("The VPL document has been modified.<p>Do you want to save the changes?"));
		msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Save);
		
		int ret = msgBox.exec();
		switch (ret)
		{
			case QMessageBox::Save:
				if (save())
					return true;
				else
					return false;
			case QMessageBox::Discard:
				return true;
			case QMessageBox::Cancel:
			default:
				return false;
		}

		return false;
	}

	QDomDocument ThymioVisualProgramming::saveToDom() const 
	{
		// if VPL is in its virign state, do not return a valid document
		// to prevent erasing of text-base code
		if (scene->isEmpty())
			return QDomDocument();
		
		QDomDocument document("tool-plugin-data");
		
		QDomElement vplroot = document.createElement("vplroot");
		document.appendChild(vplroot);

		QDomElement settings = document.createElement("settings");
		settings.setAttribute("advanced-mode", scene->getAdvanced() ? "true" : "false");
		settings.setAttribute("color-scheme", QString::number(colorComboButton->currentIndex()));
		vplroot.appendChild(settings);
		vplroot.appendChild(document.createTextNode("\n\n"));
		
		for (Scene::PairConstItr itr(scene->pairsBegin()); 
			  itr != scene->pairsEnd(); ++itr )
		{
			QDomElement element = document.createElement("buttonset");
			
			if( (*itr)->hasEventCard() ) 
			{
				const Card *card((*itr)->getEventCard());
				element.setAttribute("event-name", card->getName() );
			
				for(unsigned i=0; i<card->valuesCount(); ++i)
					element.setAttribute( QString("eb%0").arg(i), card->getValue(i));
				element.setAttribute("state", card->getStateFilter());
			}
			
			if( (*itr)->hasActionCard() ) 
			{
				const Card *card((*itr)->getActionCard());
				element.setAttribute("action-name", card->getName() );
				for(unsigned i=0; i<card->valuesCount(); ++i)
					element.setAttribute(QString("ab%0").arg(i), card->getValue(i));
			}
			
			vplroot.appendChild(element);
			vplroot.appendChild(document.createTextNode("\n\n"));
		}

		scene->setModified(false);

		return document;
	}

	void ThymioVisualProgramming::loadFromDom(const QDomDocument& document, bool fromFile) 
	{
		scene->clear();

		QDomNode domNode = document.documentElement().firstChild();

		while (!domNode.isNull())
		{
			if (domNode.isElement())
			{
				QDomElement element = domNode.toElement();
				if (element.tagName() == "settings") 
				{
					toggleAdvancedMode(element.attribute("advanced-mode") == "true", true);
					
					colorComboButton->setCurrentIndex(element.attribute("color-scheme").toInt());
				}
				else if(element.tagName() == "buttonset")
				{
					QString cardName;
					Card *eventCard = 0;
					Card *actionCard = 0;
					
					if( !(cardName = element.attribute("event-name")).isEmpty() )
					{
						eventCard = Card::createCard(cardName,scene->getAdvanced());
						if (!eventCard)
						{
							QMessageBox::warning(this,tr("Loading"),
												 tr("Error in XML source file: %0 unknown event type").arg(cardName));
							return;
						}

						for (unsigned i=0; i<eventCard->valuesCount(); ++i)
							eventCard->setValue(i,element.attribute(QString("eb%0").arg(i)).toInt());
						eventCard->setStateFilter(element.attribute("state").toInt());
					}
					
					if( !(cardName = element.attribute("action-name")).isEmpty() )
					{
						actionCard = Card::createCard(cardName,scene->getAdvanced());
						if (!actionCard)
						{
							QMessageBox::warning(this,tr("Loading"),
												 tr("Error in XML source file: %0 unknown event type").arg(cardName));
							return;
						}

						for (unsigned i=0; i<actionCard->valuesCount(); ++i)
							actionCard->setValue(i,element.attribute(QString("ab%0").arg(i)).toInt());
					}

					scene->addEventActionPair(eventCard, actionCard);
				}
			}
			domNode = domNode.nextSibling();
		}
		
		scene->haveAtLeastAnEmptyCard();
		scene->setModified(!fromFile);
		
		if (!scene->isEmpty())
			show();
	}

	void ThymioVisualProgramming::processCompilationResult()
	{
		compilationResult->setText(scene->getErrorMessage());
		if( scene->isSuccessful() ) 
		{
			compilationResultImage->setPixmap(QPixmap(QString(":/images/ok.png")));
			de->displayCode(scene->getCode(), scene->getFocusItemId());
			runButton->setEnabled(true);
			emit compilationOutcome(true);
		}
		else
		{
			compilationResultImage->setPixmap(QPixmap(QString(":/images/warning.png")));
			runButton->setEnabled(false);
			emit compilationOutcome(false);
		}
	}

	void ThymioVisualProgramming::addButtonsEvent()
	{
		ArrowButtonsEventCard *button = new ArrowButtonsEventCard(0, scene->getAdvanced());
		//scene->setFocus();
		view->centerOn(scene->addEvent(button));
	}

	void ThymioVisualProgramming::addProxEvent()
	{
		ProxEventCard *button = new ProxEventCard(0, scene->getAdvanced());
		//scene->setFocus();
		view->centerOn(scene->addEvent(button));
	}	

	void ThymioVisualProgramming::addProxGroundEvent()
	{
		ProxGroundEventCard *button = new ProxGroundEventCard(0, scene->getAdvanced());
		//scene->setFocus();
		view->centerOn(scene->addEvent(button));
	}	
	
	void ThymioVisualProgramming::addTapEvent()
	{
		TapEventCard *button = new TapEventCard(0, scene->getAdvanced());
		//scene->setFocus();
		view->centerOn(scene->addEvent(button));
	}
	
	void ThymioVisualProgramming::addClapEvent()
	{
		ClapEventCard *button = new ClapEventCard(0, scene->getAdvanced());
		//scene->setFocus();
		view->centerOn(scene->addEvent(button));
	}
	
	void ThymioVisualProgramming::addMoveAction()
	{
		MoveActionCard *button = new MoveActionCard();
		//scene->setFocus();
		view->centerOn(scene->addAction(button));
	}
	
	void ThymioVisualProgramming::addColorTopAction()
	{
		ColorActionCard *button = new TopColorActionCard();
		//scene->setFocus();
		view->centerOn(scene->addAction(button));
	}
	
	void ThymioVisualProgramming::addColorBottomAction()
	{
		ColorActionCard *button = new BottomColorActionCard();
		//scene->setFocus();
		view->centerOn(scene->addAction(button));
	}

	void ThymioVisualProgramming::addSoundAction()
	{
		SoundActionCard *button = new SoundActionCard();
		//scene->setFocus();
		view->centerOn(scene->addAction(button));
	}

	void ThymioVisualProgramming::addMemoryAction()
	{
		StateFilterActionCard *button = new StateFilterActionCard();
		//scene->setFocus();
		view->centerOn(scene->addAction(button));
	}
	
	float ThymioVisualProgramming::computeScale(QResizeEvent *event, int desiredToolbarIconSize)
	{
		// desired sizes for height
		const int idealContentHeight(5*256);
		const int uncompressibleHeight(
			actionsLabel->height() +
			desiredToolbarIconSize + 2 * style()->pixelMetric(QStyle::PM_ToolBarFrameWidth) +
			eventsLabel->height() +
			5 * style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing) +
			2 * style()->pixelMetric(QStyle::PM_LayoutTopMargin) + 
			2 * style()->pixelMetric(QStyle::PM_LayoutBottomMargin) +
			2 * 20
		);
		const int availableHeight(event->size().height() - uncompressibleHeight);
		const qreal scaleHeight(qreal(availableHeight)/qreal(idealContentHeight));
		
		// desired sizes for width
		const int idealContentWidth(1038+256*2);
		const int uncompressibleWidth(
			2 * style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing) +
			style()->pixelMetric(QStyle::PM_LayoutLeftMargin) + 
			style()->pixelMetric(QStyle::PM_LayoutRightMargin) +
			#ifdef ANDROID
			40 + 
			#else // ANDROID
			style()->pixelMetric(QStyle::PM_ScrollBarSliderMin) +
			#endif // ANDROID
			2 * 20
		);
		const int availableWidth(event->size().width() - uncompressibleWidth);
		const qreal scaleWidth(qreal(availableWidth)/qreal(idealContentWidth));
		
		// compute and set scale
		const qreal scale(qMin(scaleHeight, scaleWidth));
		return scale;
	}
	
	void ThymioVisualProgramming::resizeEvent( QResizeEvent *event)
	{
		// compute size of elements for toolbar
		const int toolbarWidgetCount(quitButton ? 10 : 9);
		const int toolbarSepCount(quitButton ? 5 : 4);
		// get width of combox box element (not content)
		QStyleOptionComboBox opt;
		QSize tmp(0, 0);
		tmp = style()->sizeFromContents(QStyle::CT_ComboBox, &opt, tmp);
		int desiredIconSize((
			event->size().width() -
			(
				(toolbarWidgetCount-1) * style()->pixelMetric(QStyle::PM_ToolBarItemSpacing) +
				(toolbarWidgetCount-1) * 2 * style()->pixelMetric(QStyle::PM_DefaultFrameWidth) + 
				2 * style()->pixelMetric(QStyle::PM_ComboBoxFrameWidth) +
				toolbarWidgetCount *  style()->pixelMetric(QStyle::PM_ButtonMargin) + 
				toolbarSepCount * style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent) +
				style()->pixelMetric(QStyle::PM_LayoutLeftMargin) +
				style()->pixelMetric(QStyle::PM_LayoutRightMargin) +
				2 * style()->pixelMetric(QStyle::PM_ToolBarItemMargin) +
				2 * style()->pixelMetric(QStyle::PM_ToolBarFrameWidth) +
				tmp.width() +
				#ifdef Q_WS_MAC
				30 // safety factor, as it seems that metrics do miss some space
				#else // Q_WS_MAC
				20
				#endif // Q_WS_MAC
				//20 // safety factor, as it seems that metrics do miss some space
			)
		) / (toolbarWidgetCount));
		
		// two pass of layout computation, should be a good-enough approximation
		qreal testScale(computeScale(event, desiredIconSize));
		desiredIconSize = qMin(desiredIconSize, int(256.*testScale));
		const qreal scale(computeScale(event, desiredIconSize));
		
		// set toolbar
		const QSize tbIconSize(QSize(desiredIconSize, desiredIconSize));
		newButton->setIconSize(tbIconSize);
		openButton->setIconSize(tbIconSize);
		saveButton->setIconSize(tbIconSize);
		saveAsButton->setIconSize(tbIconSize);
		runButton->setIconSize(tbIconSize);
		stopButton->setIconSize(tbIconSize);
		colorComboButton->setIconSize(tbIconSize);
		advancedButton->setIconSize(tbIconSize);
		helpButton->setIconSize(tbIconSize);
		if (quitButton)
			quitButton->setIconSize(tbIconSize);
		toolBar->setIconSize(tbIconSize);
		
		// set view and cards on sides
		const QSize iconSize(256*scale, 256*scale);
		view->resetTransform();
		view->scale(scale, scale);
		for(QList<CardButton*>::iterator itr = eventButtons.begin();
			itr != eventButtons.end(); ++itr)
			(*itr)->setIconSize(iconSize);
		for(QList<CardButton*>::iterator itr = actionButtons.begin();
			itr != actionButtons.end(); ++itr)
			(*itr)->setIconSize(iconSize);
	}
} } // namespace ThymioVPL / namespace Aseba
