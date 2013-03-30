#pragma once

#include <vtkCellData.h>
#include <vtkSmartPointer.h>
#include <vtkCellArray.h>
#include <vtkProperty2D.h>
#include <vtkMapper2D.h>
#include <vtkLeaderActor2D.h>
#include <vtkAlgorithmOutput.h>


//////////////////////////////////////////////////////////////////////////////////////////////
inline bool pcl::visualization::PCLVisualizer::addPointCloudNormals (const cv::Mat &cloud, const cv::Mat& normals, int level, float scale, const std::string &id, int viewport)
{
    CV_Assert(cloud.size() == normals.size() && cloud.type() == CV_32FC3 && normals.type() == CV_32FC3);

  if (cloud_actor_map_->find (id) != cloud_actor_map_->end ())
    return (false);

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();

  points->SetDataTypeToFloat ();
  vtkSmartPointer<vtkFloatArray> data = vtkSmartPointer<vtkFloatArray>::New ();
  data->SetNumberOfComponents (3);

  vtkIdType nr_normals = 0;
  float* pts = 0;

  // If the cloud is organized, then distribute the normal step in both directions
  if (cloud.cols > 1 && cloud.rows > 1)
  {
    vtkIdType point_step = static_cast<vtkIdType> (sqrt (double (level)));
    nr_normals = (static_cast<vtkIdType> ((cloud.cols - 1)/ point_step) + 1) *
                 (static_cast<vtkIdType> ((cloud.rows - 1) / point_step) + 1);
    pts = new float[2 * nr_normals * 3];

    vtkIdType cell_count = 0;
    for (vtkIdType y = 0; y < cloud.rows; y += point_step)
      for (vtkIdType x = 0; x < cloud.cols; x += point_step)
      {
        cv::Point3f p = cloud.at<cv::Point3f>(y, x);
        cv::Point3f n = normals.at<cv::Point3f>(y, x) * scale;

        pts[2 * cell_count * 3 + 0] = p.x;
        pts[2 * cell_count * 3 + 1] = p.y;
        pts[2 * cell_count * 3 + 2] = p.z;
        pts[2 * cell_count * 3 + 3] = p.x + n.x;
        pts[2 * cell_count * 3 + 4] = p.y + n.y;
        pts[2 * cell_count * 3 + 5] = p.z + n.z;

        lines->InsertNextCell (2);
        lines->InsertCellPoint (2 * cell_count);
        lines->InsertCellPoint (2 * cell_count + 1);
        cell_count++;
    }
  }
  else
  {
    nr_normals = (cloud.size().area() - 1) / level + 1 ;
    pts = new float[2 * nr_normals * 3];

    for (vtkIdType i = 0, j = 0; j < nr_normals; j++, i = j * level)
    {
      cv::Point3f p = cloud.ptr<cv::Point3f>()[i];
      cv::Point3f n = normals.ptr<cv::Point3f>()[i] * scale;

      pts[2 * j * 3 + 0] = p.x;
      pts[2 * j * 3 + 1] = p.y;
      pts[2 * j * 3 + 2] = p.z;
      pts[2 * j * 3 + 3] = p.x + n.x;
      pts[2 * j * 3 + 4] = p.y + n.y;
      pts[2 * j * 3 + 5] = p.z + n.z;

      lines->InsertNextCell (2);
      lines->InsertCellPoint (2 * j);
      lines->InsertCellPoint (2 * j + 1);
    }
  }

  data->SetArray (&pts[0], 2 * nr_normals * 3, 0);
  points->SetData (data);

  vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints (points);
  polyData->SetLines (lines);

  vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New ();
  mapper->SetInput (polyData);
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUsePointData();

  // create actor
  vtkSmartPointer<vtkLODActor> actor = vtkSmartPointer<vtkLODActor>::New ();
  actor->SetMapper (mapper);

  // Add it to all renderers
  addActorToRenderer (actor, viewport);

  // Save the pointer/ID pair to the global actor map
  (*cloud_actor_map_)[id].actor = actor;
  return (true);
}




////////////////////////////////////////////////////////////////////////////////////////////
template <typename PointT> bool
pcl::visualization::PCLVisualizer::addPolygon (
  const typename pcl::PointCloud<PointT>::ConstPtr &cloud,
  double r, double g, double b, const std::string &id, int viewport)
{
  vtkSmartPointer<vtkDataSet> data = createPolygon<PointT> (cloud);
  if (!data)
    return (false);

  // Check to see if this ID entry already exists (has it been already added to the visualizer?)
  ShapeActorMap::iterator am_it = shape_actor_map_->find (id);
  if (am_it != shape_actor_map_->end ())
  {
    vtkSmartPointer<vtkAppendPolyData> all_data = vtkSmartPointer<vtkAppendPolyData>::New ();

    // Add old data
    all_data->AddInput (reinterpret_cast<vtkPolyDataMapper*> ((vtkActor::SafeDownCast (am_it->second))->GetMapper ())->GetInput ());

    // Add new data
    vtkSmartPointer<vtkDataSetSurfaceFilter> surface_filter = vtkSmartPointer<vtkDataSetSurfaceFilter>::New ();
    surface_filter->SetInput (vtkUnstructuredGrid::SafeDownCast (data));
    vtkSmartPointer<vtkPolyData> poly_data = surface_filter->GetOutput ();
    all_data->AddInput (poly_data);

    // Create an Actor
    vtkSmartPointer<vtkActor> actor;
    createActorFromVTKDataSet (all_data->GetOutput (), actor);
    actor->GetProperty ()->SetRepresentationToWireframe ();
    actor->GetProperty ()->SetColor (r, g, b);
    actor->GetMapper ()->ScalarVisibilityOff ();
    removeActorFromRenderer (am_it->second, viewport);
    addActorToRenderer (actor, viewport);

    // Save the pointer/ID pair to the global actor map
    (*shape_actor_map_)[id] = actor;
  }
  else
  {
    // Create an Actor
    vtkSmartPointer<vtkActor> actor;
    createActorFromVTKDataSet (data, actor);
    actor->GetProperty ()->SetRepresentationToWireframe ();
    actor->GetProperty ()->SetColor (r, g, b);
    actor->GetMapper ()->ScalarVisibilityOff ();
    addActorToRenderer (actor, viewport);

    // Save the pointer/ID pair to the global actor map
    (*shape_actor_map_)[id] = actor;
  }

  return (true);
}

////////////////////////////////////////////////////////////////////////////////////////////
template <typename PointT> bool
pcl::visualization::PCLVisualizer::addPolygon (
  const pcl::PlanarPolygon<PointT> &polygon,
  double r, double g, double b, const std::string &id, int viewport)
{
  vtkSmartPointer<vtkDataSet> data = createPolygon<PointT> (polygon);
  if (!data)
    return (false);

  // Check to see if this ID entry already exists (has it been already added to the visualizer?)
  ShapeActorMap::iterator am_it = shape_actor_map_->find (id);
  if (am_it != shape_actor_map_->end ())
  {
    vtkSmartPointer<vtkAppendPolyData> all_data = vtkSmartPointer<vtkAppendPolyData>::New ();

    // Add old data
    all_data->AddInput (reinterpret_cast<vtkPolyDataMapper*> ((vtkActor::SafeDownCast (am_it->second))->GetMapper ())->GetInput ());

    // Add new data
    vtkSmartPointer<vtkDataSetSurfaceFilter> surface_filter = vtkSmartPointer<vtkDataSetSurfaceFilter>::New ();
    surface_filter->SetInput (vtkUnstructuredGrid::SafeDownCast (data));
    vtkSmartPointer<vtkPolyData> poly_data = surface_filter->GetOutput ();
    all_data->AddInput (poly_data);

    // Create an Actor
    vtkSmartPointer<vtkActor> actor;
    createActorFromVTKDataSet (all_data->GetOutput (), actor);
    actor->GetProperty ()->SetRepresentationToWireframe ();
    actor->GetProperty ()->SetColor (r, g, b);
    actor->GetMapper ()->ScalarVisibilityOn ();
    actor->GetProperty ()->BackfaceCullingOff ();
    removeActorFromRenderer (am_it->second, viewport);
    addActorToRenderer (actor, viewport);

    // Save the pointer/ID pair to the global actor map
    (*shape_actor_map_)[id] = actor;
  }
  else
  {
    // Create an Actor
    vtkSmartPointer<vtkActor> actor;
    createActorFromVTKDataSet (data, actor);
    actor->GetProperty ()->SetRepresentationToWireframe ();
    actor->GetProperty ()->SetColor (r, g, b);
    actor->GetMapper ()->ScalarVisibilityOn ();
    actor->GetProperty ()->BackfaceCullingOff ();
    addActorToRenderer (actor, viewport);

    // Save the pointer/ID pair to the global actor map
    (*shape_actor_map_)[id] = actor;
  }
  return (true);
}

////////////////////////////////////////////////////////////////////////////////////////////
template <typename PointT> bool
pcl::visualization::PCLVisualizer::addPolygon (
  const typename pcl::PointCloud<PointT>::ConstPtr &cloud,
  const std::string &id, int viewport)
{
  return (!addPolygon<PointT> (cloud, 0.5, 0.5, 0.5, id, viewport));
}

////////////////////////////////////////////////////////////////////////////////////////////
template <typename P1, typename P2> bool
pcl::visualization::PCLVisualizer::addLine (const P1 &pt1, const P2 &pt2, double r, double g, double b, const std::string &id, int viewport)
{
  // Check to see if this ID entry already exists (has it been already added to the visualizer?)
  ShapeActorMap::iterator am_it = shape_actor_map_->find (id);
  if (am_it != shape_actor_map_->end ())
  {
    PCL_WARN ("[addLine] A shape with id <%s> already exists! Please choose a different id and retry.\n", id.c_str ());
    return (false);
  }

  vtkSmartPointer<vtkDataSet> data = createLine (pt1.getVector4fMap (), pt2.getVector4fMap ());

  // Create an Actor
  vtkSmartPointer<vtkLODActor> actor;
  createActorFromVTKDataSet (data, actor);
  actor->GetProperty ()->SetRepresentationToWireframe ();
  actor->GetProperty ()->SetColor (r, g, b);
  actor->GetMapper ()->ScalarVisibilityOff ();
  addActorToRenderer (actor, viewport);

  // Save the pointer/ID pair to the global actor map
  (*shape_actor_map_)[id] = actor;
  return (true);
}

////////////////////////////////////////////////////////////////////////////////////////////
template <typename P1, typename P2> bool
pcl::visualization::PCLVisualizer::addArrow (const P1 &pt1, const P2 &pt2, double r, double g, double b, const std::string &id, int viewport)
{
  // Check to see if this ID entry already exists (has it been already added to the visualizer?)
  ShapeActorMap::iterator am_it = shape_actor_map_->find (id);
  if (am_it != shape_actor_map_->end ())
  {
    PCL_WARN ("[addArrow] A shape with id <%s> already exists! Please choose a different id and retry.\n", id.c_str ());
    return (false);
  }

  // Create an Actor
  vtkSmartPointer<vtkLeaderActor2D> leader = vtkSmartPointer<vtkLeaderActor2D>::New ();
  leader->GetPositionCoordinate ()->SetCoordinateSystemToWorld ();
  leader->GetPositionCoordinate ()->SetValue (pt1.x, pt1.y, pt1.z);
  leader->GetPosition2Coordinate ()->SetCoordinateSystemToWorld ();
  leader->GetPosition2Coordinate ()->SetValue (pt2.x, pt2.y, pt2.z);
  leader->SetArrowStyleToFilled ();
  leader->AutoLabelOn ();

  leader->GetProperty ()->SetColor (r, g, b);
  addActorToRenderer (leader, viewport);

  // Save the pointer/ID pair to the global actor map
  (*shape_actor_map_)[id] = leader;
  return (true);
}

////////////////////////////////////////////////////////////////////////////////////////////
template <typename P1, typename P2> bool
pcl::visualization::PCLVisualizer::addArrow (const P1 &pt1, const P2 &pt2, double r, double g, double b, bool display_length, const std::string &id, int viewport)
{
  // Check to see if this ID entry already exists (has it been already added to the visualizer?)
  ShapeActorMap::iterator am_it = shape_actor_map_->find (id);
  if (am_it != shape_actor_map_->end ())
  {
    PCL_WARN ("[addArrow] A shape with id <%s> already exists! Please choose a different id and retry.\n", id.c_str ());
    return (false);
  }

  // Create an Actor
  vtkSmartPointer<vtkLeaderActor2D> leader = vtkSmartPointer<vtkLeaderActor2D>::New ();
  leader->GetPositionCoordinate ()->SetCoordinateSystemToWorld ();
  leader->GetPositionCoordinate ()->SetValue (pt1.x, pt1.y, pt1.z);
  leader->GetPosition2Coordinate ()->SetCoordinateSystemToWorld ();
  leader->GetPosition2Coordinate ()->SetValue (pt2.x, pt2.y, pt2.z);
  leader->SetArrowStyleToFilled ();
  leader->SetArrowPlacementToPoint1 ();
  if (display_length)
    leader->AutoLabelOn ();
  else
    leader->AutoLabelOff ();

  leader->GetProperty ()->SetColor (r, g, b);
  addActorToRenderer (leader, viewport);

  // Save the pointer/ID pair to the global actor map
  (*shape_actor_map_)[id] = leader;
  return (true);
}
////////////////////////////////////////////////////////////////////////////////////////////
template <typename P1, typename P2> bool
pcl::visualization::PCLVisualizer::addArrow (const P1 &pt1, const P2 &pt2,
                         double r_line, double g_line, double b_line,
                         double r_text, double g_text, double b_text,
                         const std::string &id, int viewport)
{
  // Check to see if this ID entry already exists (has it been already added to the visualizer?)
  ShapeActorMap::iterator am_it = shape_actor_map_->find (id);
  if (am_it != shape_actor_map_->end ())
  {
    PCL_WARN ("[addArrow] A shape with id <%s> already exists! Please choose a different id and retry.\n", id.c_str ());
    return (false);
  }

  // Create an Actor
  vtkSmartPointer<vtkLeaderActor2D> leader = vtkSmartPointer<vtkLeaderActor2D>::New ();
  leader->GetPositionCoordinate ()->SetCoordinateSystemToWorld ();
  leader->GetPositionCoordinate ()->SetValue (pt1.x, pt1.y, pt1.z);
  leader->GetPosition2Coordinate ()->SetCoordinateSystemToWorld ();
  leader->GetPosition2Coordinate ()->SetValue (pt2.x, pt2.y, pt2.z);
  leader->SetArrowStyleToFilled ();
  leader->AutoLabelOn ();

  leader->GetLabelTextProperty()->SetColor(r_text, g_text, b_text);

  leader->GetProperty ()->SetColor (r_line, g_line, b_line);
  addActorToRenderer (leader, viewport);

  // Save the pointer/ID pair to the global actor map
  (*shape_actor_map_)[id] = leader;
  return (true);
}

////////////////////////////////////////////////////////////////////////////////////////////
template <typename P1, typename P2> bool
pcl::visualization::PCLVisualizer::addLine (const P1 &pt1, const P2 &pt2, const std::string &id, int viewport)
{
  return (!addLine (pt1, pt2, 0.5, 0.5, 0.5, id, viewport));
}

////////////////////////////////////////////////////////////////////////////////////////////
/*template <typename P1, typename P2> bool
  pcl::visualization::PCLVisualizer::addLineToGroup (const P1 &pt1, const P2 &pt2, const std::string &group_id, int viewport)
{
  vtkSmartPointer<vtkDataSet> data = createLine<P1, P2> (pt1, pt2);

  // Check to see if this ID entry already exists (has it been already added to the visualizer?)
  ShapeActorMap::iterator am_it = shape_actor_map_->find (group_id);
  if (am_it != shape_actor_map_->end ())
  {
    // Get the old data
    vtkPolyDataMapper* mapper = static_cast<vtkPolyDataMapper*>(am_it->second->GetMapper ());
    vtkPolyData* olddata = static_cast<vtkPolyData*>(mapper->GetInput ());
    // Add it to the current data
    vtkSmartPointer<vtkAppendPolyData> alldata = vtkSmartPointer<vtkAppendPolyData>::New ();
    alldata->AddInput (olddata);

    // Convert to vtkPolyData
    vtkSmartPointer<vtkPolyData> curdata = (vtkPolyData::SafeDownCast (data));
    alldata->AddInput (curdata);

    mapper->SetInput (alldata->GetOutput ());

    am_it->second->SetMapper (mapper);
    am_it->second->Modified ();
    return (true);
  }

  // Create an Actor
  vtkSmartPointer<vtkLODActor> actor;
  createActorFromVTKDataSet (data, actor);
  actor->GetProperty ()->SetRepresentationToWireframe ();
  actor->GetProperty ()->SetColor (1, 0, 0);
  addActorToRenderer (actor, viewport);

  // Save the pointer/ID pair to the global actor map
  (*shape_actor_map_)[group_id] = actor;

//ShapeActorMap::iterator am_it = shape_actor_map_->find (id);
//vtkSmartPointer<vtkLODActor> actor = am_it->second;
  //actor->GetProperty ()->SetColor (r, g, b);
//actor->Modified ();
  return (true);
}*/

////////////////////////////////////////////////////////////////////////////////////////////
inline bool pcl::visualization::PCLVisualizer::addSphere (const pcl::PointXYZ& center, double radius, double r, double g, double b, const std::string &id, int viewport)
{
  // Check to see if this ID entry already exists (has it been already added to the visualizer?)
  ShapeActorMap::iterator am_it = shape_actor_map_->find (id);
  if (am_it != shape_actor_map_->end ())
  {
    PCL_WARN ("[addSphere] A shape with id <%s> already exists! Please choose a different id and retry.\n", id.c_str ());
    return (false);
  }

  //vtkSmartPointer<vtkDataSet> data = createSphere (center.getVector4fMap (), radius);
  vtkSmartPointer<vtkSphereSource> data = vtkSmartPointer<vtkSphereSource>::New ();
  data->SetRadius (radius);
  data->SetCenter (double (center.x), double (center.y), double (center.z));
  data->SetPhiResolution (10);
  data->SetThetaResolution (10);
  data->LatLongTessellationOff ();
  data->Update ();

  // Setup actor and mapper
  vtkSmartPointer <vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New ();
  mapper->SetInputConnection (data->GetOutputPort ());

  // Create an Actor
  vtkSmartPointer<vtkLODActor> actor = vtkSmartPointer<vtkLODActor>::New ();
  actor->SetMapper (mapper);
  //createActorFromVTKDataSet (data, actor);
  actor->GetProperty ()->SetRepresentationToSurface ();
  actor->GetProperty ()->SetInterpolationToFlat ();
  actor->GetProperty ()->SetColor (r, g, b);
  actor->GetMapper ()->ImmediateModeRenderingOn ();
  actor->GetMapper ()->StaticOn ();
  actor->GetMapper ()->ScalarVisibilityOff ();
  actor->GetMapper ()->Update ();
  addActorToRenderer (actor, viewport);

  // Save the pointer/ID pair to the global actor map
  (*shape_actor_map_)[id] = actor;
  return (true);
}

////////////////////////////////////////////////////////////////////////////////////////////
inline bool pcl::visualization::PCLVisualizer::updateSphere (const pcl::PointXYZ &center, double radius, double r, double g, double b, const std::string &id)
{
  // Check to see if this ID entry already exists (has it been already added to the visualizer?)
  ShapeActorMap::iterator am_it = shape_actor_map_->find (id);
  if (am_it == shape_actor_map_->end ())
    return (false);

  //////////////////////////////////////////////////////////////////////////
  // Get the actor pointer
  vtkLODActor* actor = vtkLODActor::SafeDownCast (am_it->second);
  vtkAlgorithm *algo = actor->GetMapper ()->GetInput ()->GetProducerPort ()->GetProducer ();
  vtkSphereSource *src = vtkSphereSource::SafeDownCast (algo);

  src->SetCenter (double (center.x), double (center.y), double (center.z));
  src->SetRadius (radius);
  src->Update ();
  actor->GetProperty ()->SetColor (r, g, b);
  actor->Modified ();

  return (true);
}

//////////////////////////////////////////////////
inline bool pcl::visualization::PCLVisualizer::addText3D (const std::string &text, const PointXYZ& position,
  double textScale, double r, double g, double b, const std::string &id, int viewport)
{
  std::string tid;
  if (id.empty ())
    tid = text;
  else
    tid = id;

  // Check to see if this ID entry already exists (has it been already added to the visualizer?)
  ShapeActorMap::iterator am_it = shape_actor_map_->find (tid);
  if (am_it != shape_actor_map_->end ())
  {
    pcl::console::print_warn (stderr, "[addText3d] A text with id <%s> already exists! Please choose a different id and retry.\n", tid.c_str ());
    return (false);
  }

  vtkSmartPointer<vtkVectorText> textSource = vtkSmartPointer<vtkVectorText>::New ();
  textSource->SetText (text.c_str());
  textSource->Update ();

  vtkSmartPointer<vtkPolyDataMapper> textMapper = vtkSmartPointer<vtkPolyDataMapper>::New ();
  textMapper->SetInputConnection (textSource->GetOutputPort ());

  // Since each follower may follow a different camera, we need different followers
  rens_->InitTraversal ();
  vtkRenderer* renderer = NULL;
  int i = 1;
  while ((renderer = rens_->GetNextItem ()) != NULL)
  {
    // Should we add the actor to all renderers or just to i-nth renderer?
    if (viewport == 0 || viewport == i)
    {
      vtkSmartPointer<vtkFollower> textActor = vtkSmartPointer<vtkFollower>::New ();
      textActor->SetMapper (textMapper);
      textActor->SetPosition (position.x, position.y, position.z);
      textActor->SetScale (textScale);
      textActor->GetProperty ()->SetColor (r, g, b);
      textActor->SetCamera (renderer->GetActiveCamera ());

      renderer->AddActor (textActor);
      renderer->Render ();

      // Save the pointer/ID pair to the global actor map. If we are saving multiple vtkFollowers
      // for multiple viewport
      std::string alternate_tid = tid;
      alternate_tid.append(i, '*');

      (*shape_actor_map_)[(viewport == 0) ? tid : alternate_tid] = textActor;
    }

    ++i;
  }

  return (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////
template <typename PointT, typename PointNT> bool
pcl::visualization::PCLVisualizer::addPointCloudNormals (const typename pcl::PointCloud<PointT>::ConstPtr &cloud, const typename pcl::PointCloud<PointNT>::ConstPtr &normals,
  int level, float scale,const std::string &id, int viewport)
{
  if (normals->points.size () != cloud->points.size ())
  {
    PCL_ERROR ("[addPointCloudNormals] The number of points differs from the number of normals!\n");
    return (false);
  }
  // Check to see if this ID entry already exists (has it been already added to the visualizer?)
  CloudActorMap::iterator am_it = cloud_actor_map_->find (id);

  if (am_it != cloud_actor_map_->end ())
  {
    PCL_WARN ("[addPointCloudNormals] A PointCloud with id <%s> already exists! Please choose a different id and retry.\n", id.c_str ());
    return (false);
  }

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();

  points->SetDataTypeToFloat ();
  vtkSmartPointer<vtkFloatArray> data = vtkSmartPointer<vtkFloatArray>::New ();
  data->SetNumberOfComponents (3);


  vtkIdType nr_normals = 0;
  float* pts = 0;

  // If the cloud is organized, then distribute the normal step in both directions
  if (cloud->isOrganized () && normals->isOrganized ())
  {
    vtkIdType point_step = static_cast<vtkIdType> (sqrt (double (level)));
    nr_normals = (static_cast<vtkIdType> ((cloud->width - 1)/ point_step) + 1) *
                 (static_cast<vtkIdType> ((cloud->height - 1) / point_step) + 1);
    pts = new float[2 * nr_normals * 3];

    vtkIdType cell_count = 0;
    for (vtkIdType y = 0; y < normals->height; y += point_step)
      for (vtkIdType x = 0; x < normals->width; x += point_step)
      {
        PointT p = (*cloud)(x, y);
        p.x += (*normals)(x, y).normal[0] * scale;
        p.y += (*normals)(x, y).normal[1] * scale;
        p.z += (*normals)(x, y).normal[2] * scale;

        pts[2 * cell_count * 3 + 0] = (*cloud)(x, y).x;
        pts[2 * cell_count * 3 + 1] = (*cloud)(x, y).y;
        pts[2 * cell_count * 3 + 2] = (*cloud)(x, y).z;
        pts[2 * cell_count * 3 + 3] = p.x;
        pts[2 * cell_count * 3 + 4] = p.y;
        pts[2 * cell_count * 3 + 5] = p.z;

        lines->InsertNextCell (2);
        lines->InsertCellPoint (2 * cell_count);
        lines->InsertCellPoint (2 * cell_count + 1);
        cell_count ++;
    }
  }
  else
  {
    nr_normals = (cloud->points.size () - 1) / level + 1 ;
    pts = new float[2 * nr_normals * 3];

    for (vtkIdType i = 0, j = 0; j < nr_normals; j++, i = j * level)
    {
      PointT p = cloud->points[i];
      p.x += normals->points[i].normal[0] * scale;
      p.y += normals->points[i].normal[1] * scale;
      p.z += normals->points[i].normal[2] * scale;

      pts[2 * j * 3 + 0] = cloud->points[i].x;
      pts[2 * j * 3 + 1] = cloud->points[i].y;
      pts[2 * j * 3 + 2] = cloud->points[i].z;
      pts[2 * j * 3 + 3] = p.x;
      pts[2 * j * 3 + 4] = p.y;
      pts[2 * j * 3 + 5] = p.z;

      lines->InsertNextCell (2);
      lines->InsertCellPoint (2 * j);
      lines->InsertCellPoint (2 * j + 1);
    }
  }

  data->SetArray (&pts[0], 2 * nr_normals * 3, 0);
  points->SetData (data);

  vtkSmartPointer<vtkPolyData> polyData = vtkSmartPointer<vtkPolyData>::New();
  polyData->SetPoints (points);
  polyData->SetLines (lines);

  vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New ();
  mapper->SetInput (polyData);
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUsePointData();

  // create actor
  vtkSmartPointer<vtkLODActor> actor = vtkSmartPointer<vtkLODActor>::New ();
  actor->SetMapper (mapper);

  // Add it to all renderers
  addActorToRenderer (actor, viewport);

  // Save the pointer/ID pair to the global actor map
  (*cloud_actor_map_)[id].actor = actor;
  return (true);
}

/////////////////////////////////////////////////////////////////////////////////////////////
template <typename PointT> bool
pcl::visualization::PCLVisualizer::addPolygonMesh (const typename pcl::PointCloud<PointT>::ConstPtr &cloud,
    const std::vector<pcl::Vertices> &vertices, const std::string &id, int viewport)
{
  if (vertices.empty () || cloud->points.empty ())
    return (false);

  CloudActorMap::iterator am_it = cloud_actor_map_->find (id);
  if (am_it != cloud_actor_map_->end ())
  {
    pcl::console::print_warn (stderr,
                                "[addPolygonMesh] A shape with id <%s> already exists! Please choose a different id and retry.\n",
                                id.c_str ());
    return (false);
  }

  int rgb_idx = -1;
  std::vector<sensor_msgs::PointField> fields;
  vtkSmartPointer<vtkUnsignedCharArray> colors;
  rgb_idx = pcl::getFieldIndex (*cloud, "rgb", fields);
  if (rgb_idx == -1)
    rgb_idx = pcl::getFieldIndex (*cloud, "rgba", fields);
  if (rgb_idx != -1)
  {
    colors = vtkSmartPointer<vtkUnsignedCharArray>::New ();
    colors->SetNumberOfComponents (3);
    colors->SetName ("Colors");

    pcl::RGB rgb_data;
    for (size_t i = 0; i < cloud->size (); ++i)
    {
      if (!isFinite (cloud->points[i]))
        continue;
      memcpy (&rgb_data, reinterpret_cast<const char*> (&cloud->points[i]) + fields[rgb_idx].offset, sizeof (pcl::RGB));
      unsigned char color[3];
      color[0] = rgb_data.r;
      color[1] = rgb_data.g;
      color[2] = rgb_data.b;
      colors->InsertNextTupleValue (color);
    }
  }

  // Create points from polyMesh.cloud
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New ();
  vtkIdType nr_points = cloud->points.size ();
  points->SetNumberOfPoints (nr_points);
  vtkSmartPointer<vtkLODActor> actor;

  // Get a pointer to the beginning of the data array
  float *data = static_cast<vtkFloatArray*> (points->GetData ())->GetPointer (0);

  int ptr = 0;
  std::vector<int> lookup;
  // If the dataset is dense (no NaNs)
  if (cloud->is_dense)
  {
    for (vtkIdType i = 0; i < nr_points; ++i, ptr += 3)
      memcpy (&data[ptr], &cloud->points[i].x, sizeof (float) * 3);
  }
  else
  {
    lookup.resize (nr_points);
    vtkIdType j = 0;    // true point index
    for (vtkIdType i = 0; i < nr_points; ++i)
    {
      // Check if the point is invalid
      if (!isFinite (cloud->points[i]))
        continue;

      lookup[i] = static_cast<int> (j);
      memcpy (&data[ptr], &cloud->points[i].x, sizeof (float) * 3);
      j++;
      ptr += 3;
    }
    nr_points = j;
    points->SetNumberOfPoints (nr_points);
  }

  // Get the maximum size of a polygon
  int max_size_of_polygon = -1;
  for (size_t i = 0; i < vertices.size (); ++i)
    if (max_size_of_polygon < static_cast<int> (vertices[i].vertices.size ()))
      max_size_of_polygon = static_cast<int> (vertices[i].vertices.size ());

  if (vertices.size () > 1)
  {
    // Create polys from polyMesh.polygons
    vtkSmartPointer<vtkCellArray> cell_array = vtkSmartPointer<vtkCellArray>::New ();
    vtkIdType *cell = cell_array->WritePointer (vertices.size (), vertices.size () * (max_size_of_polygon + 1));
    int idx = 0;
    if (lookup.size () > 0)
    {
      for (size_t i = 0; i < vertices.size (); ++i, ++idx)
      {
        size_t n_points = vertices[i].vertices.size ();
        *cell++ = n_points;
        //cell_array->InsertNextCell (n_points);
        for (size_t j = 0; j < n_points; j++, ++idx)
          *cell++ = lookup[vertices[i].vertices[j]];
          //cell_array->InsertCellPoint (lookup[vertices[i].vertices[j]]);
      }
    }
    else
    {
      for (size_t i = 0; i < vertices.size (); ++i, ++idx)
      {
        size_t n_points = vertices[i].vertices.size ();
        *cell++ = n_points;
        //cell_array->InsertNextCell (n_points);
        for (size_t j = 0; j < n_points; j++, ++idx)
          *cell++ = vertices[i].vertices[j];
          //cell_array->InsertCellPoint (vertices[i].vertices[j]);
      }
    }
    vtkSmartPointer<vtkPolyData> polydata;
    allocVtkPolyData (polydata);
    cell_array->GetData ()->SetNumberOfValues (idx);
    cell_array->Squeeze ();
    polydata->SetStrips (cell_array);
    polydata->SetPoints (points);

    if (colors)
      polydata->GetPointData ()->SetScalars (colors);

    createActorFromVTKDataSet (polydata, actor, false);
  }
  else
  {
    vtkSmartPointer<vtkPolygon> polygon = vtkSmartPointer<vtkPolygon>::New ();
    size_t n_points = vertices[0].vertices.size ();
    polygon->GetPointIds ()->SetNumberOfIds (n_points - 1);

    if (lookup.size () > 0)
    {
      for (size_t j = 0; j < (n_points - 1); ++j)
        polygon->GetPointIds ()->SetId (j, lookup[vertices[0].vertices[j]]);
    }
    else
    {
      for (size_t j = 0; j < (n_points - 1); ++j)
        polygon->GetPointIds ()->SetId (j, vertices[0].vertices[j]);
    }
    vtkSmartPointer<vtkUnstructuredGrid> poly_grid;
    allocVtkUnstructuredGrid (poly_grid);
    poly_grid->Allocate (1, 1);
    poly_grid->InsertNextCell (polygon->GetCellType (), polygon->GetPointIds ());
    poly_grid->SetPoints (points);
    poly_grid->Update ();
    if (colors)
      poly_grid->GetPointData ()->SetScalars (colors);

    createActorFromVTKDataSet (poly_grid, actor, false);
  }
  addActorToRenderer (actor, viewport);
  actor->GetProperty ()->SetRepresentationToSurface ();
  // Backface culling renders the visualization slower, but guarantees that we see all triangles
  actor->GetProperty ()->BackfaceCullingOff ();
  actor->GetProperty ()->SetInterpolationToFlat ();
  actor->GetProperty ()->EdgeVisibilityOff ();
  actor->GetProperty ()->ShadingOff ();

  // Save the pointer/ID pair to the global actor map
  (*cloud_actor_map_)[id].actor = actor;
  //if (vertices.size () > 1)
  //  (*cloud_actor_map_)[id].cells = static_cast<vtkPolyDataMapper*>(actor->GetMapper ())->GetInput ()->GetVerts ()->GetData ();

  // Save the viewpoint transformation matrix to the global actor map
  vtkSmartPointer<vtkMatrix4x4> transformation = vtkSmartPointer<vtkMatrix4x4>::New();
  convertToVtkMatrix (cloud->sensor_origin_, cloud->sensor_orientation_, transformation);
  (*cloud_actor_map_)[id].viewpoint_transformation_ = transformation;

  return (true);
}

/////////////////////////////////////////////////////////////////////////////////////////////
template <typename PointT> bool pcl::visualization::PCLVisualizer::updatePolygonMesh (const typename pcl::PointCloud<PointT>::ConstPtr &cloud,
    const std::vector<pcl::Vertices> &verts, const std::string &id)
{
  if (verts.empty ())
  {
     pcl::console::print_error ("[addPolygonMesh] No vertices given!\n");
     return (false);
  }

  // Check to see if this ID entry already exists (has it been already added to the visualizer?)
  CloudActorMap::iterator am_it = cloud_actor_map_->find (id);
  if (am_it == cloud_actor_map_->end ())
    return (false);

  // Get the current poly data
  vtkSmartPointer<vtkPolyData> polydata = static_cast<vtkPolyDataMapper*>(am_it->second.actor->GetMapper ())->GetInput ();
  if (!polydata)
    return (false);
  vtkSmartPointer<vtkCellArray> cells = polydata->GetStrips ();
  if (!cells)
    return (false);
  vtkSmartPointer<vtkPoints> points   = polydata->GetPoints ();
  // Copy the new point array in
  vtkIdType nr_points = cloud->points.size ();
  points->SetNumberOfPoints (nr_points);

  // Get a pointer to the beginning of the data array
  float *data = (static_cast<vtkFloatArray*> (points->GetData ()))->GetPointer (0);

  int ptr = 0;
  std::vector<int> lookup;
  // If the dataset is dense (no NaNs)
  if (cloud->is_dense)
  {
    for (vtkIdType i = 0; i < nr_points; ++i, ptr += 3)
      memcpy (&data[ptr], &cloud->points[i].x, sizeof (float) * 3);
  }
  else
  {
    lookup.resize (nr_points);
    vtkIdType j = 0;    // true point index
    for (vtkIdType i = 0; i < nr_points; ++i)
    {
      // Check if the point is invalid
      if (!isFinite (cloud->points[i]))
        continue;

      lookup [i] = static_cast<int> (j);
      memcpy (&data[ptr], &cloud->points[i].x, sizeof (float) * 3);
      j++;
      ptr += 3;
    }
    nr_points = j;
    points->SetNumberOfPoints (nr_points);
  }

  // Update colors
  vtkUnsignedCharArray* colors = vtkUnsignedCharArray::SafeDownCast (polydata->GetPointData ()->GetScalars ());
  int rgb_idx = -1;
  std::vector<sensor_msgs::PointField> fields;
  rgb_idx = pcl::getFieldIndex (*cloud, "rgb", fields);
  if (rgb_idx == -1)
    rgb_idx = pcl::getFieldIndex (*cloud, "rgba", fields);
  if (rgb_idx != -1 && colors)
  {
    pcl::RGB rgb_data;
    int j = 0;
    for (size_t i = 0; i < cloud->size (); ++i)
    {
      if (!isFinite (cloud->points[i]))
        continue;
      memcpy (&rgb_data,
              reinterpret_cast<const char*> (&cloud->points[i]) + fields[rgb_idx].offset,
              sizeof (pcl::RGB));
      unsigned char color[3];
      color[0] = rgb_data.r;
      color[1] = rgb_data.g;
      color[2] = rgb_data.b;
      colors->SetTupleValue (j++, color);
    }
  }

  // Get the maximum size of a polygon
  int max_size_of_polygon = -1;
  for (size_t i = 0; i < verts.size (); ++i)
    if (max_size_of_polygon < static_cast<int> (verts[i].vertices.size ()))
      max_size_of_polygon = static_cast<int> (verts[i].vertices.size ());

  // Update the cells
  cells = vtkSmartPointer<vtkCellArray>::New ();
  vtkIdType *cell = cells->WritePointer (verts.size (), verts.size () * (max_size_of_polygon + 1));
  int idx = 0;
  if (lookup.size () > 0)
  {
    for (size_t i = 0; i < verts.size (); ++i, ++idx)
    {
      size_t n_points = verts[i].vertices.size ();
      *cell++ = n_points;
      for (size_t j = 0; j < n_points; j++, cell++, ++idx)
        *cell = lookup[verts[i].vertices[j]];
    }
  }
  else
  {
    for (size_t i = 0; i < verts.size (); ++i, ++idx)
    {
      size_t n_points = verts[i].vertices.size ();
      *cell++ = n_points;
      for (size_t j = 0; j < n_points; j++, cell++, ++idx)
        *cell = verts[i].vertices[j];
    }
  }
  cells->GetData ()->SetNumberOfValues (idx);
  cells->Squeeze ();
  // Set the the vertices
  polydata->SetStrips (cells);
  polydata->Update ();

/*
  vtkSmartPointer<vtkLODActor> actor;
  if (vertices.size () > 1)
  {
  }
  else
  {
    vtkSmartPointer<vtkPolygon> polygon = vtkSmartPointer<vtkPolygon>::New ();
    size_t n_points = vertices[0].vertices.size ();
    polygon->GetPointIds ()->SetNumberOfIds (n_points - 1);

    for (size_t j = 0; j < (n_points - 1); ++j)
      polygon->GetPointIds ()->SetId (j, vertices[0].vertices[j]);

    vtkSmartPointer<vtkUnstructuredGrid> poly_grid;
    allocVtkUnstructuredGrid (poly_grid);
    poly_grid->Allocate (1, 1);
    poly_grid->InsertNextCell (polygon->GetCellType (), polygon->GetPointIds ());
    poly_grid->SetPoints (points);
    poly_grid->Update ();

    createActorFromVTKDataSet (poly_grid, actor);
  }
*/

  // Get the colors from the handler
  //vtkSmartPointer<vtkDataArray> scalars;
  //color_handler.getColor (scalars);
  //polydata->GetPointData ()->SetScalars (scalars);
  //polydata->Update ();

  //am_it->second.actor->GetProperty ()->BackfaceCullingOn ();
  //am_it->second.actor->Modified ();

  return (true);
}
