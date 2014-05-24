# Copyright 1999-2014 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI=5

inherit base toolchain-funcs cmake-utils python java-pkg-opt-2 java-ant-2

DESCRIPTION="Some tools for image manipulation by means of the OpenCV library"
HOMEPAGE="https://bitbucket.org/osmanov/imtools"
# Bitbucket's tag auto URI
SRC_URI="https://bitbucket.org/osmanov/imtools/get/${P}.tar.gz"
DOCS="README.md LICENSE"

LICENSE="GNU GPLv2"
SLOT="0"
KEYWORDS="~amd64"
IUSE="debug +threads extra"

DEPEND="
	media-libs/opencv
	sys-libs/glibc
"
RDEPEND="${DEPEND}"

src_configure() {
	local mycmakeargs=(
	  $(cmake-utils_use_with debug)
	  $(cmake-utils_use_with threads)
	  $(cmake-utils_use_with extra)
	)

	cmake-utils_src_configure
}
