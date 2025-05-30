# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: Nicolas Carion <french.ebook.lover@gmail.com>, Jean-Baptiste Mardelle <jb@kdenlive.org>

set(kdenlive_SRCS
  ${kdenlive_SRCS}
  assets/assetlist/view/asseticonprovider.cpp
  assets/assetlist/view/assetlistwidget.cpp
  assets/assetlist/model/assetfilter.cpp
  assets/assetlist/model/assettreemodel.cpp
  assets/assetpanel.cpp
  assets/bpoint.cpp
  assets/keyframes/model/automask/automaskhelper.cpp
  assets/keyframes/model/keyframemonitorhelper.cpp
  assets/keyframes/model/rotoscoping/rotohelper.cpp
  assets/keyframes/model/corners/cornershelper.cpp
  assets/keyframes/model/rect/recthelper.cpp
  assets/keyframes/model/keyframemodel.cpp
  assets/keyframes/model/keyframemodellist.cpp
  assets/keyframes/view/keyframeview.cpp
  assets/model/assetparametermodel.cpp
  assets/model/assetcommand.cpp
  assets/view/assetparameterview.cpp
  assets/view/widgets/abstractparamwidget.cpp
  assets/view/widgets/boolparamwidget.cpp
  assets/view/widgets/buttonparamwidget.cpp
  assets/view/widgets/switchparamwidget.cpp
  assets/view/widgets/multiswitchparamwidget.cpp
  assets/view/widgets/effectbuttonsparamwidget.cpp
  assets/view/widgets/urllistparamwidget.cpp
  assets/view/widgets/urlparamwidget.cpp
  assets/view/widgets/doubleparamwidget.cpp
  assets/view/widgets/colorwheel.cpp
  assets/view/widgets/slidewidget.cpp
  assets/view/widgets/lumaliftgainparam.cpp
  assets/view/widgets/keyframeimport.cpp
  assets/view/widgets/keyframecontainer.cpp
  assets/view/widgets/listparamwidget.cpp
  assets/view/widgets/listdependencyparamwidget.cpp
  assets/view/widgets/geometryeditwidget.cpp
  assets/view/widgets/positioneditwidget.cpp
  assets/view/widgets/coloreditwidget.cpp
  assets/view/widgets/hideparamwidget.cpp
  assets/view/widgets/clickablelabelwidget.cpp
  assets/view/widgets/fontparamwidget.cpp
  assets/view/widgets/keywordparamwidget.cpp
  # curves widget
  assets/view/widgets/curves/abstractcurvewidget.h
  assets/view/widgets/curves/bezier/beziersplineeditor.cpp
  assets/view/widgets/curves/bezier/cubicbezierspline.cpp
  assets/view/widgets/curves/keyframe/keyframecurveeditor.cpp
  assets/view/widgets/curves/cubic/kis_curve_widget.cpp
  assets/view/widgets/curves/cubic/kis_cubic_curve.cpp
  PARENT_SCOPE)


