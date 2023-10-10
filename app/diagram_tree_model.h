#ifndef DIAGRAM_TREE_MODEL_H
#define DIAGRAM_TREE_MODEL_H
/*  Root/
 *    Diagram/
 *      Layer/
 *        Object/  object or group with objects?
 *          (Meta-)Properties/ ?
 *
 * But for a tree the concept of column and hierarchy are orthogonal, 
 * e.g. we can have a name column (something we can provide a value for)
 * for all of the objects. 
 * We may also want to build another hierarchy by connected? Or maybe
 * not because they could be circular.
 *
 * Another idea: split models, e.g. one for the application list of diagrams,
 * one for a single diagram and one for meta/properties of an object.
 */
typedef enum {
  DIAGRAM_COLUMN, /*!< conceptionally the Diagram although it is called DiagramData */
  LAYER_COLUMN, /*!< not a gobject yet, but a pointer */
  OBJECT_COLUMN, /*!< not a gobject yet, but a pointer */
  NAME_COLUMN, /*!< the name of the 'row' be it diagram/layer/object */
  NUM_COLUMNS /*! must be last - total number */
} DiaNodeType;

GtkTreeModel *diagram_tree_model_new (void);

#endif