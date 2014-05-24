# Copyright 1999-2014 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI=5

inherit base cmake-utils git-r3

DESCRIPTION="Some tools for image manipulation by means of the OpenCV library"
HOMEPAGE="https://bitbucket.org/osmanov/imtools/"
EGIT_REPO_URI="http://bitbucket.org/osmanov/${PN}.git"

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

src_configure() {
	local mycmakeargs=(
	  -DCMAKE_INSTALL_PREFIX=/usr
	  $(cmake-utils_use_with debug)
	  $(cmake-utils_use_with threads)
	  $(cmake-utils_use_with extra)
	)

	if use debug; then
		mycmakeargs+=( "-DIMTOOLS_DEBUG=ON" )
		CMAKE_BUILD_TYPE="Debug"
	fi

	cmake-utils_src_configure
}
