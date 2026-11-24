cmake_minimum_required(VERSION 3.16)
project(ecos_de_derry LANGUAGES CXX)

# IT: Ecos de Derry — roguelike acadêmico (Algoritmos e Programação II, UNIVALI).
# Três alvos (SDD §1.1):
#   regras          biblioteca estática SEM Windows: tipos, mundo, entidades, combate, jogo
#   ecos_de_derry   executável Win32: plataforma, render, cenas + regras
#   testes_regras   executável de console que testa as regras sem abrir janela

if(NOT WIN32)
    message(FATAL_ERROR "Este projeto usa Win32/GDI e compila apenas no Windows (MinGW ou MSVC).")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(PASTAS_REGRAS core mundo entidades combate jogo)
set(PASTAS_APP plataforma render cenas)

# ---------- fronteira regras × plataforma (verificada ao configurar) ----------
# As regras precisam compilar e ser testadas sem janela. Se alguém incluir um cabeçalho
# do Windows nelas, a configuração falha aqui com a explicação.
set(REGRAS_SRC "")
foreach(pasta ${PASTAS_REGRAS})
    file(GLOB_RECURSE arquivos CONFIGURE_DEPENDS
         "${CMAKE_SOURCE_DIR}/src/${pasta}/*.cpp" "${CMAKE_SOURCE_DIR}/src/${pasta}/*.h")
    foreach(arq ${arquivos})
        file(STRINGS "${arq}" proibidos REGEX "#include[ \t]*<(windows|gdiplus|xinput|mmsystem)\\.h>")
        if(proibidos)
            message(FATAL_ERROR "Fronteira violada: ${arq} inclui um cabeçalho do Windows. "
                                "Regras não podem depender da plataforma (SDD §2).")
        endif()
        if(arq MATCHES "\\.cpp$")
            list(APPEND REGRAS_SRC "${arq}")
        endif()
    endforeach()
endforeach()

add_library(regras STATIC ${REGRAS_SRC})
target_include_directories(regras PUBLIC "${CMAKE_SOURCE_DIR}/src")

set(APP_SRC "${CMAKE_SOURCE_DIR}/src/main.cpp")
foreach(pasta ${PASTAS_APP})
    file(GLOB_RECURSE arquivos CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/${pasta}/*.cpp")
    list(APPEND APP_SRC ${arquivos})
endforeach()

add_executable(ecos_de_derry WIN32 ${APP_SRC})
target_link_libraries(ecos_de_derry PRIVATE regras gdiplus gdi32 user32 winmm dwmapi shell32)
target_compile_definitions(ecos_de_derry PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)

file(GLOB TESTES_SRC CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/tests/*.cpp")
add_executable(testes_regras ${TESTES_SRC})
target_link_libraries(testes_regras PRIVATE regras)
target_compile_definitions(testes_regras PRIVATE
    ECOS_PASTA_FONTES="${CMAKE_SOURCE_DIR}/src"
    ECOS_PASTA_DADOS="${CMAKE_SOURCE_DIR}")

foreach(alvo regras ecos_de_derry testes_regras)
    if(MINGW)
        target_compile_options(${alvo} PRIVATE -Wall -Wextra -pedantic)
    elseif(MSVC)
        target_compile_options(${alvo} PRIVATE /W4 /utf-8)
    endif()
endforeach()
if(MINGW)
    # Executáveis autossuficientes: não exigem as DLLs do MinGW na máquina do professor.
    target_link_options(ecos_de_derry PRIVATE -static -municode)
    target_link_options(testes_regras PRIVATE -static)
endif()

# Assets e dados ficam ao lado do executável (caminhos relativos, PRD §24).
# Alvo que roda em TODO build (não só quando o exe é religado): assim um asset regerado
# pelas ferramentas Python chega ao jogo mesmo sem mudança no código.
add_custom_target(copiar_dados ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets/jogo" "$<TARGET_FILE_DIR:ecos_de_derry>/assets/jogo"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets/jogo_hd" "$<TARGET_FILE_DIR:ecos_de_derry>/assets/jogo_hd"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets/musics" "$<TARGET_FILE_DIR:ecos_de_derry>/assets/musics"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:ecos_de_derry>/data"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/data" "$<TARGET_FILE_DIR:ecos_de_derry>/data"
    # SDL2 2.30.9 oficial (licença zlib): carregada em tempo de execução para o joystick (SDD §6).
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/third_party/SDL2/SDL2.dll"
            "${CMAKE_SOURCE_DIR}/third_party/SDL2/README-SDL2.txt" "$<TARGET_FILE_DIR:ecos_de_derry>"
    COMMENT "Copiando assets, data e SDL2.dll para a pasta do executável")
add_dependencies(copiar_dados ecos_de_derry)

enable_testing()
add_test(NAME regras COMMAND testes_regras)
