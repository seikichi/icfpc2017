#!groovy

def list = [
  'pass',
  'random',
  'greedy',
]

pipeline {
  agent any

  options {
    buildDiscarder(logRotator(numToKeepStr: '30'))
    timeout(time: 1, unit: 'HOURS')
    timestamps()
  }

  stages {
    stage('lib') {
      steps {
        dir('lib') {
          sh 'make clean'
          sh 'make test'
        }
      }
    }

    stage('simulator') {
      steps {
        dir('simulator') {
          sh 'make clean'
          sh 'make'
          sh 'make test'
        }
      }
    }

    script {
      for (def ai : list) {
        stage("ai:${it}") {
          steps {
            dir(it) {
              sh 'make clean'
              sh 'make'
            }
          }
        }
      }
    }
  }

  post {
    failure {
      slackSend(color: 'danger', message: "BUILD FAILED <${env.BUILD_URL}console|Jenkins Console>")
    }
  }
}
