<template>
    <v-app id="inspire">
        <v-app-bar app clipped-left height="50">
            <v-toolbar-title>Lasers</v-toolbar-title>
        </v-app-bar>

        <v-content>
            <Main/>
        </v-content>
    </v-app>
</template>

<style lang="sass">
    @import 'fonts.css'
</style>

<script>
    import Main from "./components/Main";

    export default {
        components: {
            "Main": Main
        },
        props: {},
        data: () => ({}),
        created() {
            this.$vuetify.theme.dark = true
        },
    }
</script>
