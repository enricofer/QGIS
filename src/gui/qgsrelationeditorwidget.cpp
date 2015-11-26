/***************************************************************************
    qgsrelationeditor.cpp
     --------------------------------------
    Date                 : 17.5.2013
    Copyright            : (C) 2013 Matthias Kuhn
    Email                : matthias at opengis dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsrelationeditorwidget.h"

#include "qgsapplication.h"
#include "qgsdistancearea.h"
#include "qgsvectordataprovider.h"
#include "qgsexpression.h"
#include "qgsfeature.h"
#include "qgsfeatureselectiondlg.h"
#include "qgsgenericfeatureselectionmanager.h"
#include "qgsrelation.h"
#include "qgsvectorlayertools.h"

#include <QHBoxLayout>
#include <QLabel>

QgsRelationEditorWidget::QgsRelationEditorWidget( QWidget* parent )
    : QgsCollapsibleGroupBox( parent )
    , mViewMode( QgsDualView::AttributeEditor )
    , mVisible( false )
{
  QVBoxLayout* topLayout = new QVBoxLayout( this );
  topLayout->setContentsMargins( 0, 9, 0, 0 );
  setLayout( topLayout );

  // buttons
  QHBoxLayout* buttonLayout = new QHBoxLayout();
  buttonLayout->setContentsMargins( 0, 0, 0, 0 );
  // toogle editing
  mToggleEditingButton = new QToolButton( this );
  mToggleEditingButton->setIcon( QgsApplication::getThemeIcon( "/mActionToggleEditing.svg" ) );
  mToggleEditingButton->setText( tr( "Toggle editing" ) );
  mToggleEditingButton->setEnabled( false );
  mToggleEditingButton->setCheckable( true );
  buttonLayout->addWidget( mToggleEditingButton );
  // save Edits
  mSaveEditsButton = new QToolButton( this );
  mSaveEditsButton->setIcon( QgsApplication::getThemeIcon( "/mActionSaveEdits.svg" ) );
  mSaveEditsButton->setText( tr( "Save layer edits" ) );
  mSaveEditsButton->setEnabled( true );
  buttonLayout->addWidget( mSaveEditsButton );
  // add feature
  mAddFeatureButton = new QToolButton( this );
  mAddFeatureButton->setIcon( QgsApplication::getThemeIcon( "/mActionAdd.svg" ) );
  mAddFeatureButton->setText( tr( "Add feature" ) );
  buttonLayout->addWidget( mAddFeatureButton );
  // delete feature
  mDeleteFeatureButton = new QToolButton( this );
  mDeleteFeatureButton->setIcon( QgsApplication::getThemeIcon( "/mActionRemove.svg" ) );
  mDeleteFeatureButton->setText( tr( "Delete feature" ) );
  buttonLayout->addWidget( mDeleteFeatureButton );
  // link feature
  mLinkFeatureButton = new QToolButton( this );
  mLinkFeatureButton->setIcon( QgsApplication::getThemeIcon( "/mActionLink.svg" ) );
  mLinkFeatureButton->setText( tr( "Link feature" ) );
  buttonLayout->addWidget( mLinkFeatureButton );
  // unlink feature
  mUnlinkFeatureButton = new QToolButton( this );
  mUnlinkFeatureButton->setIcon( QgsApplication::getThemeIcon( "/mActionUnlink.svg" ) );
  mUnlinkFeatureButton->setText( tr( "Unlink feature" ) );
  buttonLayout->addWidget( mUnlinkFeatureButton );
  // spacer
  buttonLayout->addItem( new QSpacerItem( 0, 0, QSizePolicy::Expanding ) );
  // form view
  mFormViewButton = new QToolButton( this );
  mFormViewButton->setText( tr( "Form view" ) );
  mFormViewButton->setIcon( QgsApplication::getThemeIcon( "/mActionPropertyItem.png" ) );
  mFormViewButton->setCheckable( true );
  mFormViewButton->setChecked( mViewMode == QgsDualView::AttributeEditor );
  buttonLayout->addWidget( mFormViewButton );
  // table view
  mTableViewButton = new QToolButton( this );
  mTableViewButton->setText( tr( "Table view" ) );
  mTableViewButton->setIcon( QgsApplication::getThemeIcon( "/mActionOpenTable.png" ) );
  mTableViewButton->setCheckable( true );
  mTableViewButton->setChecked( mViewMode == QgsDualView::AttributeTable );
  buttonLayout->addWidget( mTableViewButton );
  // button group
  mViewModeButtonGroup = new QButtonGroup( this );
  mViewModeButtonGroup->addButton( mFormViewButton, QgsDualView::AttributeEditor );
  mViewModeButtonGroup->addButton( mTableViewButton, QgsDualView::AttributeTable );

  // add buttons layout
  topLayout->addLayout( buttonLayout );

  // Set initial state for add/remove etc. buttons
  updateButtons();

  mRelationLayout = new QGridLayout();
  mRelationLayout->setContentsMargins( 0, 0, 0, 0 );
  topLayout->addLayout( mRelationLayout );

  mDualView = new QgsDualView( this );
  mDualView->setView( mViewMode );
  mFeatureSelectionMgr = new QgsGenericFeatureSelectionManager( mDualView );
  mDualView->setFeatureSelectionManager( mFeatureSelectionMgr );

  mRelationLayout->addWidget( mDualView );

  connect( this, SIGNAL( collapsedStateChanged( bool ) ), this, SLOT( onCollapsedStateChanged( bool ) ) );
  connect( mViewModeButtonGroup, SIGNAL( buttonClicked( int ) ), this, SLOT( setViewMode( int ) ) );
  connect( mToggleEditingButton, SIGNAL( clicked( bool ) ), this, SLOT( toggleEditing( bool ) ) );
  connect( mSaveEditsButton, SIGNAL( clicked() ), this, SLOT( saveEdits() ) );
  connect( mAddFeatureButton, SIGNAL( clicked() ), this, SLOT( addFeature() ) );
  connect( mDeleteFeatureButton, SIGNAL( clicked() ), this, SLOT( deleteFeature() ) );
  connect( mLinkFeatureButton, SIGNAL( clicked() ), this, SLOT( linkFeature() ) );
  connect( mUnlinkFeatureButton, SIGNAL( clicked() ), this, SLOT( unlinkFeature() ) );
}

void QgsRelationEditorWidget::setRelationFeature( const QgsRelation& relation, const QgsFeature& feature )
{
  if ( mRelation.isValid() )
  {
    disconnect( mRelation.referencingLayer(), SIGNAL( editingStarted() ), this, SLOT( updateButtons() ) );
    disconnect( mRelation.referencingLayer(), SIGNAL( editingStopped() ), this, SLOT( updateButtons() ) );
  }

  mRelation = relation;
  mFeature = feature;

  connect( mRelation.referencingLayer(), SIGNAL( editingStarted() ), this, SLOT( updateButtons() ) );
  connect( mRelation.referencingLayer(), SIGNAL( editingStopped() ), this, SLOT( updateButtons() ) );

  setTitle( relation.name() );

  QgsVectorLayer* lyr = relation.referencingLayer();

  bool canChangeAttributes = lyr->dataProvider()->capabilities() & QgsVectorDataProvider::ChangeAttributeValues;
  if ( canChangeAttributes && !lyr->isReadOnly() )
  {
    mToggleEditingButton->setEnabled( true );
    updateButtons();
  }
  else
  {
    mToggleEditingButton->setEnabled( false );
  }

  setObjectName( mRelation.name() );
  loadState();

  // If not yet initialized, it is not (yet) visible, so we don't load it to be faster (lazy loading)
  // If it is already initialized, it has been set visible before and the currently shown feature is changing
  // and the widget needs updating

  if ( mVisible )
  {
    QgsFeatureRequest myRequest = mRelation.getRelatedFeaturesRequest( mFeature );

    mDualView->init( mRelation.referencingLayer(), 0, myRequest, mEditorContext );
  }
}

void QgsRelationEditorWidget::setRelations( const QgsRelation& relation, const QgsRelation& nmrelation )
{
  if ( mRelation.isValid() )
  {
    disconnect( mRelation.referencingLayer(), SIGNAL( editingStarted() ), this, SLOT( updateButtons() ) );
    disconnect( mRelation.referencingLayer(), SIGNAL( editingStopped() ), this, SLOT( updateButtons() ) );
  }

  if ( mNmRelation.isValid() )
  {
    disconnect( mNmRelation.referencingLayer(), SIGNAL( editingStarted() ), this, SLOT( updateButtons() ) );
    disconnect( mNmRelation.referencingLayer(), SIGNAL( editingStopped() ), this, SLOT( updateButtons() ) );
  }

  mRelation = relation;
  mNmRelation = nmrelation;

  connect( mRelation.referencingLayer(), SIGNAL( editingStarted() ), this, SLOT( updateButtons() ) );
  connect( mRelation.referencingLayer(), SIGNAL( editingStopped() ), this, SLOT( updateButtons() ) );

  if ( mNmRelation.isValid() )
  {
    connect( mNmRelation.referencingLayer(), SIGNAL( editingStarted() ), this, SLOT( updateButtons() ) );
    connect( mNmRelation.referencingLayer(), SIGNAL( editingStopped() ), this, SLOT( updateButtons() ) );
  }

  setTitle( relation.name() );

  QgsVectorLayer* lyr = relation.referencingLayer();

  bool canChangeAttributes = lyr->dataProvider()->capabilities() & QgsVectorDataProvider::ChangeAttributeValues;
  if ( canChangeAttributes && !lyr->isReadOnly() )
  {
    mToggleEditingButton->setEnabled( true );
    updateButtons();
  }
  else
  {
    mToggleEditingButton->setEnabled( false );
  }

  setObjectName( mRelation.name() );
  loadState();

  updateUi();
}

void QgsRelationEditorWidget::setEditorContext( const QgsAttributeEditorContext& context )
{
  mEditorContext = context;
}

void QgsRelationEditorWidget::setViewMode( QgsDualView::ViewMode mode )
{
  mDualView->setView( mode );
  mViewMode = mode;
}


void QgsRelationEditorWidget::setFeature( const QgsFeature& feature )
{
  mFeature = feature;

  updateUi();
}

void QgsRelationEditorWidget::updateButtons()
{
  bool editable = false;
  bool linkable = false;
  if ( mRelation.isValid() )
  {
    editable = mRelation.referencingLayer()->isEditable();
    linkable = mRelation.referencingLayer()->isEditable();
  }

  if ( mNmRelation.isValid() )
  {
    editable = mNmRelation.referencedLayer()->isEditable();
  }

  mAddFeatureButton->setEnabled( editable );
  mLinkFeatureButton->setEnabled( linkable );
  mDeleteFeatureButton->setEnabled( editable );
  mUnlinkFeatureButton->setEnabled( linkable );
  mToggleEditingButton->setChecked( editable );
  mSaveEditsButton->setEnabled( editable );
}

void QgsRelationEditorWidget::addFeature()
{
  QgsAttributeMap keyAttrs;

  QgsFields fields = mRelation.referencingLayer()->fields();

  Q_FOREACH ( const QgsRelation::FieldPair& fieldPair, mRelation.fieldPairs() )
  {
    keyAttrs.insert( fields.indexFromName( fieldPair.referencingField() ), mFeature.attribute( fieldPair.referencedField() ) );
  }

  mEditorContext.vectorLayerTools()->addFeature( mDualView->masterModel()->layer(), keyAttrs );
}

void QgsRelationEditorWidget::linkFeature()
{
  QgsFeatureSelectionDlg selectionDlg( mRelation.referencingLayer(), mEditorContext , this );

  if ( selectionDlg.exec() )
  {
    QMap<int, QVariant> keys;
    Q_FOREACH ( const QgsRelation::FieldPair& fieldPair, mRelation.fieldPairs() )
    {
      int idx = mRelation.referencingLayer()->fieldNameIndex( fieldPair.referencingField() );
      QVariant val = mFeature.attribute( fieldPair.referencedField() );
      keys.insert( idx, val );
    }

    Q_FOREACH ( QgsFeatureId fid, selectionDlg.selectedFeatures() )
    {
      QMapIterator<int, QVariant> it( keys );
      while ( it.hasNext() )
      {
        it.next();
        mRelation.referencingLayer()->changeAttributeValue( fid, it.key(), it.value() );
      }
    }
  }
}

void QgsRelationEditorWidget::deleteFeature()
{
  Q_FOREACH ( QgsFeatureId fid, mFeatureSelectionMgr->selectedFeaturesIds() )
  {
    mRelation.referencingLayer()->deleteFeature( fid );
  }
}

void QgsRelationEditorWidget::unlinkFeature()
{
  if ( mNmRelation.isValid() )
  {

  }
  else
  {
    QMap<int, QgsField> keyFields;
    Q_FOREACH ( const QgsRelation::FieldPair& fieldPair, mRelation.fieldPairs() )
    {
      int idx = mRelation.referencingLayer()->fieldNameIndex( fieldPair.referencingField() );
      if ( idx < 0 )
      {
        QgsDebugMsg( QString( "referencing field %1 not found" ).arg( fieldPair.referencingField() ) );
        return;
      }
      QgsField fld = mRelation.referencingLayer()->fields().at( idx );
      keyFields.insert( idx, fld );
    }

    Q_FOREACH ( QgsFeatureId fid, mFeatureSelectionMgr->selectedFeaturesIds() )
    {
      QMapIterator<int, QgsField> it( keyFields );
      while ( it.hasNext() )
      {
        it.next();
        mRelation.referencingLayer()->changeAttributeValue( fid, it.key(), QVariant( it.value().type() ) );
      }
    }
  }
}

void QgsRelationEditorWidget::toggleEditing( bool state )
{
  if ( state )
  {
    mEditorContext.vectorLayerTools()->startEditing( mRelation.referencingLayer() );
    if ( mNmRelation.isValid() )
      mEditorContext.vectorLayerTools()->startEditing( mNmRelation.referencedLayer() );
  }
  else
  {
    mEditorContext.vectorLayerTools()->stopEditing( mRelation.referencingLayer() );
    if ( mNmRelation.isValid() )
      mEditorContext.vectorLayerTools()->stopEditing( mNmRelation.referencedLayer() );
  }
}

void QgsRelationEditorWidget::saveEdits()
{
  mEditorContext.vectorLayerTools()->saveEdits( mRelation.referencingLayer() );
  if ( mNmRelation.isValid() )
    mEditorContext.vectorLayerTools()->saveEdits( mNmRelation.referencedLayer() );
}

void QgsRelationEditorWidget::onCollapsedStateChanged( bool collapsed )
{
  if ( !collapsed )
    mVisible = true;

  updateUi();
}

void QgsRelationEditorWidget::updateUi()
{
  // If not yet initialized, it is not (yet) visible, so we don't load it to be faster (lazy loading)
  // If it is already initialized, it has been set visible before and the currently shown feature is changing
  // and the widget needs updating

  if ( mVisible )
  {
    QgsFeatureRequest myRequest = mRelation.getRelatedFeaturesRequest( mFeature );

    if ( mNmRelation.isValid() )
    {
      QgsFeatureIterator it = mRelation.referencingLayer()->getFeatures( myRequest );

      QgsFeature fet;
      QgsFeatureIds ids;

      while ( it.nextFeature( fet ) )
      {
        ids << fet.id();
      }

      QgsFeatureRequest nmRequest;

      nmRequest.setFilterFids( ids );

      mDualView->init( mNmRelation.referencedLayer(), 0, nmRequest, mEditorContext );
    }
    else
    {
      mDualView->init( mRelation.referencingLayer(), 0, myRequest, mEditorContext );
    }
  }
}
