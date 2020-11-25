#pragma once

//#define GODOT_3
#define GODOT_4

#ifdef GODOT_4
// visual server becomes rendering server
#define GD_VS RS

#define GD_RD RenderingDevice

//#define GD_COMMAND_LINE CommandPrimitive
#else

#define GD_VS VS

// no rendering device in 3.2?
#define GD_RD VS

//#define GD_COMMAND_LINE CommandLine

#define GD_TYPE_LINE TYPE_LINE
#define GD_TYPE_POLYLINE TYPE_POLYLINE
#define GD_TYPE_POLYGON TYPE_POLYGON
#define GD_TYPE_CIRCLE TYPE_CIRCLE
#endif
