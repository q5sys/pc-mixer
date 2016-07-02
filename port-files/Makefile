# Created by: Joe Maloney <jmaloney@ixsystems.com>
# $FreeBSD$

PORTNAME=	pc-mixer
PORTVERSION=	%%CHGVERSION%%
CATEGORIES=	audio

MAINTAINER=	jmaloney@ixsystems.com
COMMENT=	QT5 based audio mixer for FreeBSD

LICENSE=	BSD2CLAUSE

WRKSRC_SUBDIR=  src-qt5
MAKE_ARGS=      PREFIX=${STAGEDIR}${PREFIX}

MAKE_JOBS_UNSAFE=yes
ONLY_FOR_ARCHS= amd64
USE_LDCONFIG=   yes
USE_QT5=        core gui network svg linguist \
                buildtools x11extras widgets
USES=           qmake shebangfix tar:xz
USE_GL+=	gl

WRKSRC_SUBDIR=  src-qt5

QMAKE_ARGS=     CONFIG+="configure"

USE_GITHUB=	yes
GH_ACCOUNT=	trueos
GH_PROJECT=	pc-mixer
GH_TAGNAME=	%%GHTAG%%

.include <bsd.port.mk>
