cxx.std = latest
using cxx

hxx{*}: extension = hpp
cxx{*}: extension = cpp

libs =
import libs += vulkan%lib{vulkan}
import libs += glfw3%lib{glfw3}

exe{main}: {hxx cxx}{**} $libs
{
  test = true
}

cxx.poptions =+ "-I$src_root"