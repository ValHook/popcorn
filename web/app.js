particlesJS.load('particles', 'particles.json');
var socket = new WebSocket("ws://"+window.location.hostname+":2200");
socket.onerror = function() {
    alert("An error occured. The page will refresh");
    window.location.reload();
}
socket.onclose = function () {
    alert("The connection was closed by the server. The page will refresh");
    window.location.reload();
}
socket.onmessage = function (evt) {
    vue.sockrecv(evt.data);
}


var vue = new Vue({
    el: '#main',
    mounted : function() {
    },
    data: {
        autocomplete: [],
        predictions: [],
        movie_title: "",
        full_movie: null,
        feature: 'd',
        query: ""
    },
    methods: {
        sockrecv: function (res) {
            if (this.feature === 'd') {
                var tmp_autocomplete = [];
                res = res.split("\n");
                var size = res.length - 1;
                for (var i = 0; i < size; i++) {
                    var movie = res[i].split("§");
                    tmp_autocomplete.push({
                        cover: movie[1],
                        title: movie[0],
                        director_date: movie[2] + " (" + movie[3] + ")"
                    });
                }
                this.autocomplete = tmp_autocomplete;
            } else if (this.feature === 'a') {
                this.predictions = JSON.parse(res);
            } else if (this.feature === 'f') {
                if (res === "") {
                    this.feature = 'd';
                } else {
                    this.full_movie = JSON.parse(res);
                    if (this.full_movie.score_viewers == -1)
                        this.full_movie.score_viewers = '?';
                    else
                        this.full_movie.score_viewers = Math.round(this.full_movie.score_viewers *10) / 10;
                    if (this.full_movie.reviews_viewers == -1)
                        this.full_movie.reviews_viewers = 0;
                    this.full_movie.duration_genre_date = "";
                    if (this.full_movie.hours != -1)
                        this.full_movie.duration_genre_date = this.full_movie.hours + "h " + this.full_movie.minutes + " min&nbsp;&nbsp|&nbsp;&nbsp;";
                    if (this.full_movie.genre.length)
                        this.full_movie.duration_genre_date += this.full_movie.genre.join(", ") + "&nbsp;&nbsp|&nbsp;&nbsp;";
                    this.full_movie.duration_genre_date += this.full_movie.date;
                }
            }
        }
    }
});