pipeline {
  agent none
  stages {
    stage('Dev') {
      steps {
        build 'Build'
      }
    }
    stage('Production') {
      steps {
        mail(subject: 'Cloudbees', body: 'Production', from: 'yourcizor@gmail.com', to: 'amrit.srivastava@tivo.com')
      }
    }
  }
  environment {
    TEST = '1'
  }
}