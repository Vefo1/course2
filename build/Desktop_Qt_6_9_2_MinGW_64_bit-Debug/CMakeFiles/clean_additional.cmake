# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\StockExchange_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\StockExchange_autogen.dir\\ParseCache.txt"
  "StockExchange_autogen"
  )
endif()
