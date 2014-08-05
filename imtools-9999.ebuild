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
IUSE="debug +threads +openmp extra"

DEPEND="
	media-libs/opencv
	sys-libs/glibc
	threads? ( !openmp? ( dev-libs/boost[threads] dev-cpp/threadpool ) )
"
RDEPEND="${DEPEND}"

src_configure() {
	use openmp && tc-has-openmp \
		|| die "Your current compiler doesn't support OpenMP"

	local mycmakeargs=(
	  -DCMAKE_INSTALL_PREFIX=/usr
	  $(cmake-utils_use debug IMTOOLS_DEBUG)
	  $(cmake-utils_use threads IMTOOLS_THREADS)
	  $(cmake-utils_use openmp IMTOOLS_THREADS_OPENMP)
	  $(cmake-utils_use extra IMTOOLS_EXTRA)
	)

	if use debug; then
		CMAKE_BUILD_TYPE="Debug"
	fi

	cmake-utils_src_configure
}
