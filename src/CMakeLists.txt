# Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

add_subdirectory(genrev)

if (BUILD_LOGON OR BUILD_WORLD OR BUILD_TOOLS)
  add_subdirectory(common)
endif()

if(BUILD_LOGON OR BUILD_WORLD)
  add_subdirectory(server)
endif()

if(BUILD_TOOLS)
  add_subdirectory(tools)
endif()
