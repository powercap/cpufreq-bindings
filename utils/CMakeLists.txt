# Binaries

add_executable(cpufreq-bindings-read-cpu cpufreq-bindings-read-cpu.c)
target_link_libraries(cpufreq-bindings-read-cpu ${PROJECT_NAME})

install(TARGETS cpufreq-bindings-read-cpu DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY man/ DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
