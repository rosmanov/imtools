# Copyright 1999-2014 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI=5

inherit base cmake-utils git-r3

DESCRIPTION="Some tools for image manipulation by means of the OpenCV library"
HOMEPAGE="https://bitbucket.org/osmanov/imtools/"
EGIT_REPO_URI="http://bitbucket.org/osmanov/${PN}.git"
#SRC_URI="mirror://bitbucket.org/osmanov/imtools/${PN}/${P}/.tar.gz"

DOCS="README.md LICENSE"
LICENSE="GPL-2"
SLOT="0"
KEYWORDS="~amd64 ~x86"
IUSE="debug +threads extra"

DEPEND="
	media-libs/opencv
	sys-libs/glibc
"
RDEPEND="${DEPEND}"

#CMAKE_IN_SOURCE_BUILD="1"

#src_prepare() {
	#base_src_prepare
	##sed -i -r 's%(set *\(CMAKE_RUNTIME_OUTPUT_DIRECTORY .*\))%%g' CMakeLists.txt || die
#}

src_configure() {
	local mycmakeargs=(
	  -DCMAKE_INSTALL_PREFIX=/usr
	  $(cmake-utils_use_with debug)
	  $(cmake-utils_use_with threads)
	  $(cmake-utils_use_with extra)
	)

	cmake-utils_src_configure
}

#src_install() {
	#cmake-utils_src_install
#}
