// ==UserScript==
// @name         VideoPlayer
// @namespace    http://fancybox.net/
// @version      0.1
// @description  try to take over the world!
// @author       You
// @include      http://localhost:9999/*
// @grant        none
// @require https://cdn.bootcss.com/fancybox/3.1.20/jquery.fancybox.js
// ==/UserScript==

var videos = ['.flv', '.mkv', '.avi', '.wmv'];
var images = ['.jpg', '.jpeg', '.png', '.git'];
var styleText = `
<style>
video {width: 100%}
.fancybox-slide--iframe .fancybox-content {width: 928px;}
</style>
`;

(function() {
    'use strict';
    $('head').append('<link  href="https://cdn.bootcss.com/fancybox/3.1.20/jquery.fancybox.css" rel="stylesheet">');
    $('head').append(styleText);
    $(document).on('beforeShow.fb', function(e, instance) {
        var src = instance.$lastFocus["0"].innerText;
        if (videos.indexOf(src.slice(-4)) !== -1) {
            instance.close();
        }
    });
    link_fancy();
    $('#example').on( 'draw.dt', function () {
        link_fancy();
    } );
})();

function link_fancy() {
    $('#example a[data-file]').each(function(i, item){
        if (typeof($(item).attr("data-fancybox")) == "undefined") {
            var filename = $(item).text();
            if (images.indexOf(filename.substring(filename.lastIndexOf('.'))) !== -1) {
                $(item).attr("data-fancybox", "images");
                $(item).attr("data-caption", $(item).text());
                $(item).attr("src", encodeURIComponent($(item).attr('href')));
            } else {
                $(item).attr("data-fancybox", "");
                $(item).attr("data-type", "iframe");
                $(item).attr("data-src", encodeURIComponent($(item).attr('href')));
                $(item).attr("href", "javascript:;");
            }

        }
    });
}
